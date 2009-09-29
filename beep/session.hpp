/// \file  session.hpp
/// \brief BEEP session
///
/// UNCLASSIFIED
#ifndef BEEP_SESSION_HEAD
#define BEEP_SESSION_HEAD 1

#include <vector>
#include <iterator>
#include <map>
#include <stdexcept>

#include <boost/noncopyable.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/system/error_code.hpp>

#include "role.hpp"
#include "frame.hpp"
#include "frame-generator.hpp"
#include "message-generator.hpp"
#include "message.hpp"
#include "channel.hpp"
#include "channel-manager.hpp"
#include "profile.hpp"

namespace beep {

template <class U> class basic_session;

namespace detail {

class handler_tuning_events {
public:
	typedef const boost::system::error_code & error_code_reference;
	typedef unsigned int message_number_type;
	typedef boost::function<void (error_code_reference)> function_type;
	typedef std::map<message_number_type, function_type> callback_container;
	typedef callback_container::iterator iterator;

	handler_tuning_events()
		: callbacks_()
	{
	}

	void add(const message_number_type num, function_type cb)
	{
		callbacks_[num] = cb;
	}

	iterator get_callback(const message_number_type num)
	{
		const iterator i = callbacks_.find(num);
		if (i == callbacks_.end()) {
			throw std::runtime_error("Invalid callback number");
		}
		return i;
	}

	void remove_callback(iterator i)
	{
		callbacks_.erase(i);
	}
private:
	callback_container callbacks_;
};     // class handler_tuning_events
}      // namespace detail

/// \brief BEEP session management
///
/// \note Each session has an implicit tuning channel that performs
///       initialization.
template <class TransportServiceT>
class basic_session
	: private boost::noncopyable
{
public:
	typedef TransportServiceT                transport_service;
	typedef transport_service&               transport_service_reference;
	typedef basic_session<transport_service> base_type;
	typedef std::size_t                      size_type;

	basic_session(transport_service_reference ts)
		: transport_(ts)
		, id_()
		, netchng_()
		, frmsig_()
		, chman_()
		, handler_()
		, profiles_()
	{
		using boost::bind;
		netchng_ =
			transport_.install_network_handler(bind(&basic_session::handle_network_change,
													this, _1, _2));
	}

	~basic_session()
	{
		frmsig_.disconnect();
		netchng_.disconnect();
	}

	//template <class Handler>
	/// \todo install a handler that is invoked when a peer starts a channel with this profile
	void install_profile(const profile &p)
	{
		profiles_[p.uri()] = p;
	}

	const identifier &id() const { return id_; }

	template <typename OutputIterator>
	size_type available_profiles(OutputIterator out)
	{
		using std::transform;
		using std::iterator_traits;
		typedef typename iterator_traits<profile_container::const_iterator>::value_type value_type;
		transform(profiles_.begin(), profiles_.end(), out,
				  cmp::detail::profile_uri_extractor<value_type>());
		return profiles_.size();
	}

	template <class Handler>
	unsigned int async_add_channel(const std::string &profile_uri, Handler handler)
	{
		using std::ostringstream;
		using boost::bind;

		ostringstream strm;
		strm << id_;
		message start;
		profile prof = get_profile(profile_uri);
		const unsigned int ch =
			chman_.start_channel(transport_service::get_role(),
								 strm.str(), prof, start);
		const unsigned int msgno = send_tuning_message(start);
		handler_.add(msgno, bind(handler, _1, ch, prof));
		return ch;
	}
private:
	void handle_network_change(const boost::system::error_code &error,
							   const identifier &id)
	{
		if (!error) {
			id_ = id;
			frmsig_ =
				transport_.subscribe(id,
									 bind(&basic_session::handle_frame,
										  this, _1, _2));
			start();
		} else {
			// try to re-establish???
			// report error?
			// throw system_error?
		}
	}

	void handle_frame(const boost::system::error_code &error, const frame &frm)
	{
		if (!error) {
			/// \todo handle messages that broken into multiple frames.
			message msg; // todo make message from frame
			make_message(&frm, &frm + 1, msg);
			if (frm.channel() == chman_.get_tuning_channel().number()) {
				handle_tuning_frame(frm, msg);
			} else {
				// distribute the message to appropriate handler
			}
		} else {
			/// \todo handle the error condition!
			frmsig_.disconnect();
		}
	}

	void handle_tuning_frame(const frame &frm, const message &msg)
	{
		using std::back_inserter;
		using std::vector;
		if (msg.get_type() == message::RPY && cmp::is_greeting_message(msg)) {
			vector<profile> new_profs;
			chman_.get_profiles(msg, back_inserter(new_profs));
			typedef vector<profile>::const_iterator iterator;
			for (iterator i = new_profs.begin(); i != new_profs.end(); ++i) {
				profiles_[i->uri()] = *i;
			}
		} else if (msg.get_type() == message::MSG && cmp::is_start_message(msg)) {
		} else if (msg.get_type() == message::MSG && cmp::is_close_message(msg)) {
			message response;
			if (chman_.close_channel(msg, response)) {
				/// \todo notify the client that its channel was closed...
			}
			send_tuning_message(response);
		} else if (msg.get_type() == message::RPY && cmp::is_ok_message(msg)) {
			boost::system::error_code message_error;
			detail::handler_tuning_events::iterator i =
				handler_.get_callback(frm.message());
			i->second(message_error);
			handler_.remove_callback(i);
		} else if (msg.get_type() == message::RPY && cmp::is_error_message(msg)) {
		} else {
			/// \todo handle other frame types
			assert(false);
		}
	}

	void start()
	{
		message greeting;
		chman_.get_greeting_message(profiles_.begin(), profiles_.end(), greeting);
		send_tuning_message(greeting);
	}
private:
	typedef typename transport_service::signal_connection   signal_connection_t;
	typedef std::map<std::string, profile>                  profile_container;
	transport_service_reference   transport_;
	identifier                    id_;
	signal_connection_t           netchng_;
	signal_connection_t           frmsig_;

	channel_manager               chman_;
	detail::handler_tuning_events handler_;
	profile_container             profiles_;

	const profile &get_profile(const std::string &uri) const
	{
		profile_container::const_iterator i = profiles_.find(uri);
		if (i == profiles_.end()) {
			throw std::runtime_error("Invalid profile!");
		}
		return i->second;
	}

	/// \return the used message number
	unsigned int send_tuning_message(const message &msg)
	{
		using std::vector;
		using std::back_inserter;

		vector<frame> frames;
		make_frames(msg, chman_.get_tuning_channel(), back_inserter(frames));
		assert(!frames.empty());
		transport_.send_frames(frames.begin(), frames.end());
		return frames.front().message();
	}
};     // class basic_session

}      // namespace beep
#endif // BEEP_SESSION_HEAD
