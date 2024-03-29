/// \file  channel-manager.cpp
/// \brief Add and remove channels from a session
///
/// UNCLASSIFIED
#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdexcept>
#include <sstream>
#include <utility>
#include <boost/bind.hpp>
#include "channel-manager.hpp"
#include "message.hpp"
#include "reply-code.hpp"

namespace beep {

channel_manager::channel_manager()
	: profiles_()
	, channels_()
	, guess_(0)
	, notifications_()
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
		std::ostringstream estrm;
		estrm << "channel_manager::prepare_message_for_channel -- The selected channel (" << ch << ") is not in use.";
		throw std::runtime_error(estrm.str().c_str());
	}
	channel &my_channel = channel_iterator->second;
	msg.set_channel(my_channel);
	/// \todo cache the payload object.
	const std::string my_payload = msg.get_payload();
	my_channel.update(my_payload.size());
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

std::pair<bool, cmp::protocol_node>
channel_manager::peer_requested_channel_close(const cmp::close_message &close_msg)
{
	using std::make_pair;
	ch_map::iterator channel_iterator = channels_.find(close_msg.channel);
	if (close_msg.channel > 0 && channel_iterator == channels_.end()) {
		cmp::error_message err;
		err.code = reply_code::requested_action_not_accepted;
		std::ostringstream estrm;
		estrm << "The requested channel(" << close_msg.channel << ") was not in use.";
		err.diagnostic = estrm.str();
		return make_pair(false, err);
	}
	const bool request_close_normal_channel = close_msg.channel != detail::tuning_channel_number();
	if (request_close_normal_channel) {
		boost::system::error_code not_an_error;
		prof_map::iterator profile_iterator =
			profiles_.find(channel_iterator->second.get_profile());
		if (profile_iterator == profiles_.end()) {
			throw std::runtime_error("The closing channel profile is missing.");
		}
		profile_iterator->second(not_an_error, close_msg.channel, true, message());
	}
	// con't erase the channel_iterator from channels_ here because the session object
	// still needs to generate an "OK" message to send to the peer. prepare_message_for_channel
	// will throw an exception if it cannot find the referenced channel number.
	return make_pair(!request_close_normal_channel, cmp::ok_message());
}

cmp::protocol_node
channel_manager::request_close_channel(const unsigned int channel, const reply_code::rc_enum rc)
{
	ch_map::iterator channel_iterator = channels_.find(channel);
	if (channel_iterator == channels_.end()) {
		std::ostringstream estrm;
		estrm << "channel_manager::request_close_channel -- invalid channel (" << channel << ") close request!";
		throw std::runtime_error(estrm.str().c_str());
	}
	cmp::close_message request;
	request.channel = channel;
	request.code = rc;
	return request;
}

void
channel_manager::close_channel(const unsigned int channel)
{
	if (0u == channels_.erase(channel)) {
		std::ostringstream estrm;
		estrm << "channel_manager::close_channel -- invalid channel (" << channel << ") close!";
		throw std::runtime_error(estrm.str().c_str());
	}
}

cmp::protocol_node
channel_manager::accept_start(const cmp::start_message &start_msg)
{
	using std::ostringstream;
	using std::ostream_iterator;
	using std::make_pair;
	using boost::bind;
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
				notifications_.push_back(bind(profile_iter->second, not_an_error, start_msg.channel, false, i->initialization));
				//(profile_iter->second)(not_an_error, start_msg.channel, false, i->initialization);
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

void
channel_manager::invoke_pending_channel_notifications()
{
	for (notifications_container::const_iterator i = notifications_.begin(); i != notifications_.end(); ++i) {
		(*i)();
	}
	notifications_.clear();
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
