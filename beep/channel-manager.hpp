/// \file  channel-manager.hpp
/// \brief Add and remove channels from a session
///
/// UNCLASSIFIED
#ifndef BEEP_CHANNEL_MANAGER_HEAD
#define BEEP_CHANNEL_MANAGER_HEAD 1

#include <cassert>
#include <stdexcept>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <utility>
#include <functional>

#include <boost/function.hpp>
#include <boost/system/error_code.hpp>

#include "role.hpp"
#include "channel.hpp"
#include "channel-management-protocol.hpp"
#include "reply-code.hpp"

namespace beep {
namespace detail {
inline unsigned int tuning_channel_number() { return 0; }
}      // namespace detail

class channel_manager {
public:
	channel_manager();

	void prepare_message_for_channel(const unsigned int ch, message &msg);

	template <class Handler>
	void install_profile(const std::string &profile_uri, Handler handler)
	{
		using std::make_pair;
		const std::pair<prof_map::iterator, bool> result =
			profiles_.insert(make_pair(profile_uri, handler));
		if (!result.second) {
			const profile_callback_type installed_callback = result.first->second;
			if (installed_callback) {
				throw std::runtime_error("The profile already exists!");
			} else {
				profiles_[profile_uri] = handler;
			}
		}
	}

	void install_profile(const std::string &profile_uri)
	{
		using std::make_pair;
		const std::pair<prof_map::iterator, bool> result =
			profiles_.insert(make_pair(profile_uri, profile_callback_type()));
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

	cmp::protocol_node get_greeting_message() const;

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
	///
	/// \return std::pair<unsigned int channel number, cmp::protocol_node request>
	std::pair<unsigned int, cmp::protocol_node>
	start_channel(const role r, const std::string &name, const std::string &profile_uri);

	/// \brief the peer requested that the channel be closed
	/// \return std::pair<bool peer requested session close, cmp::protocol_node response>
	std::pair<bool, cmp::protocol_node>
	peer_requested_channel_close(const cmp::close_message &close_msg);

	cmp::protocol_node close_channel(const unsigned int channel, const reply_code::rc_enum rc);

	/// \return Accepted channel number, zero indicates an error
	cmp::protocol_node accept_start(const cmp::start_message &start_msg);

	/// \brief temporary hack
	///
	/// This function is used to invoke any queued notifications that a new channel was created.
	/// This function exists because the notifications cannot be invoked directly within the
	/// \em accept_start member function. If so, any wire traffic would be sent before the channel
	/// accept message (tuning channel). This is not easy to fix currently. It should be easier
	/// after I implement channel transmission priorities.
	///
	/// For now, invoke this function after a call to accept_start and after you transmit the
	/// channel start-up response.
	void invoke_pending_channel_notifications();
private:
	typedef std::map<unsigned int, channel> ch_map;
	typedef boost::system::error_code error_code;
	typedef boost::function<void (const error_code&, unsigned, bool, const message&)> profile_callback_type;
	typedef std::map<std::string, profile_callback_type> prof_map;

	prof_map     profiles_;
	ch_map       channels_;
	unsigned int guess_; // guess at next channel number

	typedef std::vector<boost::function<void ()> > notifications_container;
	notifications_container notifications_; // pending notifications of channel start-up

	unsigned int get_next_channel(const role r);

	struct profile_pair_to_uri : public std::unary_function<prof_map::value_type, std::string> {
		std::string operator()(const prof_map::value_type &pair) const
		{
			return pair.first;
		}
	};
};

}      // namespace beep
#endif // BEEP_CHANNEL_MANAGER_HEAD
