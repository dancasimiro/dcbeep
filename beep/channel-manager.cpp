/// \file  channel-manager.cpp
/// \brief Add and remove channels from a session
///
/// UNCLASSIFIED
#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdexcept>
#include <utility>
#include "channel-manager.hpp"
#include "message.hpp"
#include "reply-code.hpp"

namespace beep {

channel_manager::channel_manager()
	: profiles_()
	, channels_()
	, guess_(0)
{
	// used for adding/removing subsequent channels
	using std::make_pair;
	const std::pair<ch_map::iterator, bool> result =
		channels_.insert(make_pair(0, channel(0)));
	if (!result.second) {
		throw std::runtime_error("Could not initialize the tuning channel!");
	}
}

void
channel_manager::prepare_message_for_channel(const unsigned int ch, message &msg)
{
	ch_map::iterator channel_iterator = channels_.find(ch);
	if (channel_iterator == channels_.end()) {
		throw std::runtime_error("the selected channel is not in use.");
	}
	channel &my_channel = channel_iterator->second;
	msg.set_channel(my_channel);
	my_channel.update(msg.get_payload_size());
}

cmp::protocol_node
channel_manager::get_greeting_message() const
{
	using std::back_inserter;
	cmp::greeting_message message;
	get_profiles(back_inserter(message.profile_uris));
	return message;
}

std::pair<unsigned int, cmp::protocol_node>
channel_manager::start_channel(const role r, const std::string &name, const std::string &profile_uri)
{
	using std::make_pair;
	if (!profiles_.count(profile_uri)) return make_pair(0, cmp::protocol_node());

	unsigned int number = guess_;
	if (!channels_.insert(make_pair(number, channel(number, profile_uri))).second) {
		number = get_next_channel(r);
		if (!channels_.insert(make_pair(number, channel(number, profile_uri))).second) {
			throw std::runtime_error("could not find a channel number!");
		}
	}
	cmp::start_message start;
	start.channel = number;
	start.server_name = name;
	start.profiles.push_back(profile_uri);
	guess_ = number + 2;
	return make_pair(number, start);
}

cmp::protocol_node
channel_manager::peer_requested_channel_close(const cmp::close_message &close_msg)
{
	ch_map::iterator channel_iterator = channels_.find(close_msg.channel);
	if (close_msg.channel > 0 && channel_iterator == channels_.end()) {
		cmp::error_message err;
		err.code = reply_code::requested_action_not_accepted;
		std::ostringstream estrm;
		estrm << "The requested channel(" << close_msg.channel << ") was not in use.";
		err.diagnostic = estrm.str();
		return err;
	}
	if (close_msg.channel != detail::tuning_channel_number()) {
		boost::system::error_code not_an_error;
		prof_map::iterator profile_iterator =
			profiles_.find(channel_iterator->second.get_profile());
		if (profile_iterator == profiles_.end()) {
			throw std::runtime_error("The closing channel profile is missing.");
		}
		profile_iterator->second(not_an_error, close_msg.channel, true, message());
	}
	channels_.erase(channel_iterator);
	return cmp::ok_message();
}

cmp::protocol_node
channel_manager::close_channel(const unsigned int channel, const reply_code::rc_enum rc)
{
	ch_map::iterator channel_iterator = channels_.find(channel);
	if (channel_iterator == channels_.end()) {
		throw std::runtime_error("invalid channel close request!");
	}
	if (channel != detail::tuning_channel_number()) {
		channels_.erase(channel_iterator);
	}
	cmp::close_message request;
	request.channel = channel;
	request.code = rc;
	return request;
}

cmp::protocol_node
channel_manager::accept_start(const cmp::start_message &start_msg)
{
	using std::ostringstream;
	using std::ostream_iterator;
	using std::make_pair;
	if (!channels_.count(start_msg.channel)) {
		prof_map::const_iterator profile_iter = profiles_.end();
		typedef std::vector<cmp::profile_element>::const_iterator start_iterator;
		for (start_iterator i = start_msg.profiles.begin(); i != start_msg.profiles.end(); ++i) {
			profile_iter = profiles_.find(i->uri);
			if (profile_iter != profiles_.end()) {
				/// \todo check return value of channels_.insert
				channels_.insert(make_pair(start_msg.channel, channel(start_msg.channel, i->uri)));
				cmp::profile_element response;
				response.uri = i->uri;
				// Execute the profile callback to tell the client that a new channel was
				// started by the peer.
				/// \note need to queue any frames that are generated from this callback until the
				///       positive response for this channel creation is sent. Use the flow
				///       management architecture.
				boost::system::error_code not_an_error;
				(profile_iter->second)(not_an_error, start_msg.channel, false, i->initialization);
				return response;
			}
		}
		assert(profile_iter == profiles_.end());
		ostringstream estrm;
		estrm << "The specified profile(s) are not supported. "
			"This listener supports the following profiles: ";
		get_profiles(ostream_iterator<std::string>(estrm, ", "));
		cmp::error_message error;
		error.code = reply_code::requested_action_not_accepted;
		error.diagnostic = estrm.str();
		return error;
	}
	ostringstream estrm;
	estrm << "The requested channel (" << start_msg.channel << ") is already in use.";
	cmp::error_message error;
	error.code = reply_code::requested_action_not_accepted;
	error.diagnostic = estrm.str();
	return error;
}

static unsigned int get_first_number(const role r)
{
	unsigned int number = 0;
	switch (r) {
	case initiating_role:
		number = 1;
		break;
	case listening_role:
		number = 2;
		break;
	default:
		throw std::runtime_error("Unknown beep role in the channel manager");
	}
	return number;
}

unsigned int
channel_manager::get_next_channel(const role r)
{
	unsigned int number = 0;
	if (channels_.empty()) {
		number = get_first_number(r);
	} else {
		number = channels_.rbegin()->first;
		if (!number) {
			number = get_first_number(r);
		} else {
			number += 2;
		}
	}
	return number;
}

} // namespace beep
