/// \file  channel-manager.hpp
/// \brief Add and remove channels from a session
///
/// UNCLASSIFIED
#ifndef BEEP_CHANNEL_MANAGER_HEAD
#define BEEP_CHANNEL_MANAGER_HEAD 1

#include <cassert>
#include <stdexcept>
#include <istream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <iterator>
#include <utility>
#include <functional>
#include <limits>

#include <boost/function.hpp>
#include <boost/system/error_code.hpp>

#include "role.hpp"
#include "frame.hpp" // for core_message_types definition
#include "channel.hpp"
#include "channel-management-protocol.hpp"
#include "reply-code.hpp"

namespace beep {
namespace detail {
inline unsigned int tuning_channel_number() { return 0; }
}      // namespace detail

class channel_manager {
public:
	channel_manager()
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

	void prepare_message_for_channel(const unsigned int ch, message &msg)
	{
		ch_map::iterator channel_iterator = channels_.find(ch);
		if (channel_iterator == channels_.end()) {
			throw std::runtime_error("the selected channel is not in use.");
		}
		channel &my_channel = channel_iterator->second;
		msg.set_channel(my_channel);
		my_channel.update(msg.get_payload_size());
	}

	template <class Handler>
	void install_profile(const std::string &profile_uri, Handler handler)
	{
		using std::make_pair;
		const std::pair<prof_map::iterator, bool> result =
			profiles_.insert(make_pair(profile_uri, handler));
		if (!result.second) {
			throw std::runtime_error("The profile was not installed.");
		}
	}

	template <typename OutputIterator>
	OutputIterator get_profiles(OutputIterator out) const
	{
		using std::transform;
		return transform(profiles_.begin(), profiles_.end(), out, profile_pair_to_uri());
	}

	template <typename FwdIterator>
	void
	greeting_message(const FwdIterator first_profile, const FwdIterator last_profile,
					 message &greeting_msg) const
	{
#if 0
		using std::ostringstream;
		using std::copy;
		using std::back_inserter;

		cmp::greeting_message g_protocol;
		copy(first_profile, last_profile, back_inserter(g_protocol.profile_uris));
		greeting_msg = cmp::generate(g_protocol);
		//greeting_msg.set_mime(mime::beep_xml());
		greeting_msg.set_type(RPY);
#endif
	}

	/// \brief test if a channel is active
	///
	/// First, test if the channel number represents the tuning channel. If not,
	/// check if the number is currently in use.
	///
	/// \return True if the channel number is currently active.
	bool channel_in_use(const unsigned int channel) const
	{
		return (channels_.count(channel) > 0);
	}

	/// To avoid conflict in assigning channel numbers when requesting the
	/// creation of a channel, BEEP peers acting in the initiating role use
	/// only positive integers that are odd-numbered; similarly, BEEP peers
	/// acting in the listening role use only positive integers that are
	/// even-numbered.
	unsigned int start_channel(const role r, const std::string &name,
							   const std::string &profile_uri, const message &msg)
	{
#if 0
		using std::ostringstream;
		using std::make_pair;
		if (!profiles_.count(profile_uri)) return 0;

		unsigned int number = guess_;
		if (!channels_.insert(make_pair(number, channel(number, profile_uri))).second) {
			number = get_next_channel(r);
			if (!channels_.insert(make_pair(number, channel(number, profile_uri))).second) {
				throw std::runtime_error("could not find a channel number!");
			}
		}
		msg.set_mime(mime::beep_xml());
		msg.set_type(MSG);
		cmp::start start;
		start.set_number(number);
		start.set_server_name(name);
		start.push_back_profile(profile_uri);
		ostringstream strm;
		strm << start;
		msg.set_content(strm.str());
		guess_ = number + 2;
		return number;
#else
		return 0;
#endif
	}

	// the peer requested that the channel be closed
	cmp::protocol_node peer_requested_channel_close(const cmp::close_message &close_msg)
	{
		ch_map::iterator channel_iterator = channels_.find(close_msg.channel);
		if (close_msg.channel > 0 && channel_iterator == channels_.end()) {
			cmp::error_message err;
			err.code = 550;
			err.diagnostic = "The requested channel was not in use.";
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

	cmp::protocol_node close_channel(const unsigned int channel, const reply_code::rc_enum rc)
	{
		ch_map::iterator channel_iterator = channels_.find(channel);
		if (channel > 0 && channel_iterator == channels_.end()) {
			throw std::runtime_error("invalid channel close request!");
		}
		channels_.erase(channel_iterator);
		cmp::close_message request;
		request.channel = channel;
		request.code = rc;
		return request;
	}

	/// \return Accepted channel number, zero indicates an error
	cmp::protocol_node accept_start(const cmp::start_message &start_msg)
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
			//response.set_type(ERR);
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

	unsigned close_channel(const message &close_msg, message &response)
	{
#if 0
		using std::istringstream;
		using std::ostringstream;
		using std::numeric_limits;

		response.set_mime(mime::beep_xml());

		cmp::close close;
		istringstream strm(close_msg.get_content());
		if (!(strm >> close)) {
			response.set_type(ERR);
			cmp::error error(reply_code::general_syntax_error,
							 "The 'close' message could not be decoded.");
			ostringstream ostrm;
			ostrm << error;
			response.set_content(ostrm.str());
			return numeric_limits<unsigned>::max();
		}
		const unsigned ch_num = close.number();
		if (channels_.erase(ch_num)) {
			response.set_type(RPY);
			cmp::ok ok;
			ostringstream ostrm;
			ostrm << ok;
			response.set_content(ostrm.str());
		} else {
			response.set_type(ERR);
			cmp::error error(reply_code::parameter_invalid,
							 "Channel closure failed because the referenced channel number is not recognized.");
			ostringstream ostrm;
			ostrm << error;
			response.set_content(ostrm.str());
			return numeric_limits<unsigned>::max();
		}
		return ch_num;
#else
		return 0;
#endif
	}
private:
	typedef std::map<unsigned int, channel> ch_map;
	typedef boost::system::error_code error_code;
	typedef boost::function<void (const error_code&, unsigned, bool, const message&)> profile_callback_type;
	typedef std::map<std::string, profile_callback_type> prof_map;

	prof_map     profiles_;
	ch_map       channels_;
	unsigned int guess_; // guess at next channel number

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

	unsigned int get_next_channel(const role r)
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

	struct profile_pair_to_uri : public std::unary_function<prof_map::value_type, std::string> {
		std::string operator()(const prof_map::value_type &pair) const
		{
			return pair.first;
		}
	};
};

}      // namespace beep
#endif // BEEP_CHANNEL_MANAGER_HEAD
