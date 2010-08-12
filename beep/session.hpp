/// \file  session.hpp
/// \brief BEEP session
///
/// UNCLASSIFIED
#ifndef BEEP_SESSION_HEAD
#define BEEP_SESSION_HEAD 1

#include <vector>
#include <map>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <cassert>

#include <boost/noncopyable.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/system/error_code.hpp>
#include <boost/signals2.hpp>
#include <boost/variant.hpp>

#include "role.hpp"
#include "frame.hpp"
#include "frame-generator.hpp"
#include "message-generator.hpp"
#include "message.hpp"
#include "channel.hpp"
#include "channel-manager.hpp"

namespace beep {
namespace detail {

struct channel_number_matcher : public std::unary_function<bool, channel> {
	channel_number_matcher(const unsigned c) : chnum_(c) {}
	virtual ~channel_number_matcher() {}

	bool operator()(const channel &c) const
	{
		return c.get_number() == chnum_;
	}

	const unsigned int chnum_;
};

template <typename FunctionT>
class basic_event_handler {
public:
	typedef const boost::system::error_code & error_code_reference;
	typedef unsigned int key_type;
	typedef FunctionT    function_type;

	basic_event_handler() : callbacks_() {}
	virtual ~basic_event_handler() {}

	void add(const key_type num, function_type cb)
	{
		assert(callbacks_.find(num) == callbacks_.end());
		callbacks_[num] = cb;
	}
protected:
	typedef std::map<key_type, function_type>     callback_container;
	typedef typename callback_container::iterator iterator;

	iterator get_callback(const key_type num)
	{
		const iterator i = callbacks_.find(num);
		assert(i != callbacks_.end());
		if (i == callbacks_.end()) {
			using std::ostringstream;
			ostringstream strm;
			strm << num << " is an invalid " << descr() << " callback number.";
			throw std::runtime_error(strm.str());
		}
		return i;
	}

	void remove_callback(const iterator i)
	{
		callbacks_.erase(i);
	}
private:
	callback_container callbacks_;

	virtual const std::string &descr() const = 0;
};

typedef boost::system::error_code error_code;

class handler_tuning_events
	: public basic_event_handler<boost::function<void (const error_code&)> > {
public:
	virtual ~handler_tuning_events() {}

	void execute(const key_type num, const error_code &error)
	{
		const iterator i = get_callback(num);
		/// copy the callback and then remove it from the list
		/// The callback may add a new callback for this channel.
		const function_type cb = i->second;
		remove_callback(i);
		/// Now, invoke the callback
		cb(error);
	}
private:
	virtual const std::string &descr() const
	{
		static const std::string d("tuning");
		return d;
	}
};     // class handler_tuning_events

class handler_user_events
	: public basic_event_handler<boost::function<void (const error_code&, const message&)> > {
public:
	virtual ~handler_user_events() {}

	void execute(const key_type channel, const error_code &error, const message &msg)
	{
		const iterator i = get_callback(channel);
		/// copy the callback and then remove it from the list
		/// The callback may add a new callback for this channel.
		const function_type cb = i->second;
		remove_callback(i);
		cb(error, msg);
	}
private:
	virtual const std::string &descr() const
	{
		static const std::string d("user");
		return d;
	}
};     // class handler_user_events

class tuning_message_visitor : public boost::static_visitor<message> {
public:
	tuning_message_visitor (channel_manager &chman) : manager_(chman) { }

	//} else if (msg.get_type() == MSG && cmp::is_start_message(msg)) {
	message operator()(const cmp::start_message &msg) const
	{
		message response;
		profile acceptedProfile;
		// send_tuning_message is in both branches because I want to send
		// "OK" message before I execute the profile handler and
		// _possibly_ send channel data.
#if 0
		if (const unsigned int chnum =
			manager_.accept_start(msg, profiles_.begin(), profiles_.end(),
								  acceptedProfile, response)) {
#if 0
			channels_.push_back(channel(chnum));
			ch2prof_.insert(make_pair(chnum, acceptedProfile.uri()));
#endif
			//send_tuning_message(response);
#if 0
			boost::system::error_code not_an_error;
			const detail::wrapped_profile &myProfile =
				get_profile(acceptedProfile.uri());
			myProfile.execute(chnum, not_an_error, acceptedProfile.initial_message(), false);
#endif
			//} else {
			//send_tuning_message(response);
		}
#endif
		return response;
	}

	// else if (msg.get_type() == MSG && cmp::is_close_message(msg)) {
	message operator()(const cmp::close_message &msg) const
	{
		message response;
#if 0
		const unsigned chnum = manager_.close_channel(msg, response);
		//send_tuning_message(response);
		if (response.get_type() == RPY) {
			assert(chnum != numeric_limits<unsigned>::max());
			if (chnum != chman_.tuning_channel().get_number()) {
				boost::system::error_code not_an_error;
				const detail::wrapped_profile &myProfile = get_profile(chnum);
				myProfile.execute(chnum, not_an_error, response, true);
				ch2prof_.erase(chnum);
			} else {
				transport_.shutdown_connection(id_);
			}
		}
#endif
		return response;
	}

	message operator()(const cmp::ok_message &) const
	{
		message response;
		// return error message...
		return response;
	}

	message operator()(const cmp::greeting_message &) const
	{
		message response;
		// return error message...
		return response;
	}

	message operator()(const cmp::error_message &) const
	{
		message response;
		// return error message...
		return response;
	}

	message operator()(const cmp::profile_element &) const
	{
		message response;
		// return error message...
		return response;
	}
private:
	channel_manager &manager_;
};     // tuning_message_visitor

class tuning_reply_visitor : public boost::static_visitor<void> {
public:
	void operator()(const cmp::greeting_message &) const
	{
	}

	void operator()(const cmp::start_message &) const
	{
	}

	void operator()(const cmp::close_message &) const
	{
	}

	void operator()(const cmp::ok_message &) const
	{
	}

	void operator()(const cmp::error_message &) const
	{
	}

	void operator()(const cmp::profile_element &) const
	{
	}
};     // tuning_reply_visitor

class tuning_error_visitor : public boost::static_visitor<void> {
public:
	void operator()(const cmp::greeting_message &) const
	{
	}

	void operator()(const cmp::start_message &) const
	{
	}

	void operator()(const cmp::close_message &) const
	{
	}

	void operator()(const cmp::ok_message &) const
	{
	}

	void operator()(const cmp::error_message &) const
	{
	}

	void operator()(const cmp::profile_element &) const
	{
	}
};     // tuning_error_visitor

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

	typedef boost::signals2::connection      signal_connection;
	typedef boost::signals2::signal<void (const boost::system::error_code&)> session_signal_t;

	template <typename U> friend void shutdown_session(basic_session<U>&);

	basic_session(transport_service_reference ts)
		: transport_(ts)
		, id_()
		, frmsig_()
		, chman_()
		, channels_()
		, ch2prof_()
		, tuning_handler_()
		, user_handler_()
		, session_signal_()
	{
	}

	basic_session(transport_service_reference ts, const identifier &id)
		: transport_(ts)
		, id_(id)
		, frmsig_()
		, chman_()
		, channels_()
		, ch2prof_()
		, tuning_handler_()
		, user_handler_()
		, session_signal_()
	{
		this->set_id(id);
	}

	~basic_session()
	{
		frmsig_.disconnect();
	}

	/// \todo add an overload that accepts an initialization message.
	template <class Handler>
	void install_profile(const std::string &profile_uri, Handler handler)
	{
		chman_.install_profile(profile_uri, handler);
	}

	signal_connection install_session_handler(const session_signal_t::slot_type slot)
	{
		return session_signal_.connect(slot);
	}

	const identifier &id() const { return id_; }

	/// Set the new identifier and associate this session with the transport
	void set_id(const identifier &id)
	{
		using boost::bind;
		frmsig_.disconnect();
		id_ = id;
		frmsig_ =
			transport_.subscribe(id,
								 bind(&basic_session::handle_frame,
									  this, _1, _2));
		start();
	}

	template <typename OutputIterator>
	OutputIterator available_profiles(OutputIterator out) const
	{
		return chman_.get_profiles(out);
	}

	template <class Handler>
	unsigned int async_add_channel(const std::string &profile_uri, Handler handler)
	{
		using std::ostringstream;
		using boost::bind;
		using std::make_pair;

		ostringstream strm;
		strm << id_;
		message start;
		if (const unsigned int ch =
			chman_.start_channel(transport_service::get_role(),
								 strm.str(), profile_uri, start)) {
			const unsigned int msgno = send_tuning_message(start);
			tuning_handler_.add(msgno, bind(handler, _1, ch, profile_uri));
			channels_.push_back(channel(ch));
			ch2prof_.insert(make_pair(ch, profile_uri));
			return ch;
		}
		return 0;
	}

	template <class Handler>
	void async_close_channel(const unsigned int channel,
							 const reply_code::rc_enum rc, Handler handler)
	{
		if (chman_.channel_in_use(channel)) {
			message close;
			chman_.close_channel(channel, rc, close);
			const unsigned int msgno = send_tuning_message(close);
			tuning_handler_.add(msgno, bind(handler, _1, channel));
			if (channel != chman_.tuning_channel().get_number()) {
				remove_channel(channel);
				ch2prof_.erase(channel);
			}
		} else {
			throw std::runtime_error("the selected channel is not in use.");
		}
	}

	template <class Handler>
	void async_read(const unsigned int channel, Handler handler)
	{
		using boost::bind;
		if (chman_.channel_in_use(channel)) {
			user_handler_.add(channel, bind(handler, _1, _2, channel));
		} else {
			throw std::runtime_error("the selected channel is not in use.");
		}
	}

	void send(const unsigned int channel, const message &msg)
	{
		if (chman_.channel_in_use(channel)) {
			send_message(msg, get_channel(channel));
		} else {
			throw std::runtime_error("the selected channel is not in use.");
		}
	}
private:
	typedef typename transport_service::signal_connection signal_connection_t;
	typedef std::vector<channel>                          channel_container;
	typedef std::map<unsigned, std::string>               channel_to_profile_map;
	typedef boost::system::error_code error_code;
	typedef boost::function<void (const error_code&, unsigned, bool, const message&)> function_type;

	transport_service_reference   transport_;
	identifier                    id_;
	signal_connection_t           frmsig_;

	channel_manager               chman_;
	channel_container             channels_;
	channel_to_profile_map        ch2prof_; // resolves channel numbers to a profile

	detail::handler_tuning_events tuning_handler_;
	detail::handler_user_events   user_handler_;

	session_signal_t              session_signal_;

	void handle_frame(const boost::system::error_code &error, const frame &frm)
	{
		if (!error) {
			/// \todo handle messages that are broken into multiple frames.
			try {
				message msg;
				make_message(&frm, &frm + 1, msg);
				if (msg.get_channel() == chman_.tuning_channel().get_number()) {
					handle_tuning_message(msg);
				} else {
					handle_user_message(msg);
				}
			} catch (const std::exception &ex) {
#if 0
				/// \todo handle the error condition!
				/// \todo Check if this is the right error condition.
				///       Maybe I should close the channel?
				message msg;
				msg.set_type(ERR);
				cmp::error myError(reply_code::requested_action_aborted,
								   ex.what());
				std::ostringstream strm;
				strm << myError;
				msg.set_content(strm.str());
				send_tuning_message(msg);
#endif
			}
		} else {
			frmsig_.disconnect();
			transport_.stop_connection(id_);
			session_signal_(error);
		}
	}

	void handle_tuning_message(const message &msg)
	{
		using boost::apply_visitor;
		const cmp::protocol_node my_node = cmp::parse(msg);
		switch (msg.get_type()) {
		case MSG:
			{
				const message response = apply_visitor(detail::tuning_message_visitor(chman_), my_node);
				send_tuning_message(response);
			}
			break;
		case RPY:
			apply_visitor(detail::tuning_reply_visitor(), my_node);
			break;
		case ERR:
			apply_visitor(detail::tuning_error_visitor(), my_node);
			break;
		case ANS:
		case NUL:
		default:
			assert(false);
			throw std::runtime_error("invalid message type in tuning handler...");
		}
#if 0
		if (msg.get_type() == RPY && cmp::is_greeting_message(msg)) {
			chman_.copy_profiles(msg, back_inserter(profiles_));
			session_signal_(boost::system::error_code());
		} else if (msg.get_type() == MSG && cmp::is_start_message(msg)) {
			message response;
			profile acceptedProfile;
			// send_tuning_message is in both branches because I want to send
			// "OK" message before I execute the profile handler and
			// _possibly_ send channel data.
			if (const unsigned int chnum =
				chman_.accept_start(msg, profiles_.begin(), profiles_.end(),
									acceptedProfile, response)) {
				channels_.push_back(channel(chnum));
				ch2prof_.insert(make_pair(chnum, acceptedProfile.uri()));
				send_tuning_message(response);
				boost::system::error_code not_an_error;
				const detail::wrapped_profile &myProfile =
					get_profile(acceptedProfile.uri());
				myProfile.execute(chnum, not_an_error, acceptedProfile.initial_message(), false);
			} else {
				send_tuning_message(response);
			}
		} else if (msg.get_type() == MSG && cmp::is_close_message(msg)) {
			message response;
			const unsigned chnum = chman_.close_channel(msg, response);
			send_tuning_message(response);
			if (response.get_type() == RPY) {
				assert(chnum != numeric_limits<unsigned>::max());
				if (chnum != chman_.tuning_channel().get_number()) {
					boost::system::error_code not_an_error;
					const detail::wrapped_profile &myProfile = get_profile(chnum);
					myProfile.execute(chnum, not_an_error, response, true);
					ch2prof_.erase(chnum);
				} else {
					transport_.shutdown_connection(id_);
				}
			}
		} else if (msg.get_type() == RPY && (cmp::is_ok_message(msg) || cmp::is_profile_message(msg))) {
			boost::system::error_code message_error;
			tuning_handler_.execute(msg.get_number(), message_error);
		} else if (msg.get_type() == ERR) {
			const boost::system::system_error error = make_error(msg);
			/// \todo log the error description (error.what())
			/// \todo should I throw the system_error error here?
			tuning_handler_.execute(msg.get_number(), error.code());
		} else {
			/// \todo handle other frame types
			std::cerr << "there was an unexpected message type:  " << msg.get_type() << std::endl;
			assert(false);
		}
#endif
	}

	void handle_user_message(const message &msg)
	{
		boost::system::error_code error;
		user_handler_.execute(msg.get_channel(), error, msg);
	}

	void start()
	{
#if 0
		message greeting;
		chman_.greeting_message(profiles_.begin(), profiles_.end(), greeting);
		send_tuning_message(greeting);
#endif
	}

	channel &get_channel(const unsigned int chnum)
	{
		using std::find_if;
		channel_container::iterator i =
			find_if(channels_.begin(), channels_.end(),
					detail::channel_number_matcher(chnum));
		if (i == channels_.end()) {
			throw std::runtime_error("Invalid channel!");
		}
		return *i;
	}

	const channel &get_channel(const unsigned int chnum) const
	{
		using std::find_if;
		channel_container::const_iterator i =
			find_if(channels_.begin(), channels_.end(),
					detail::channel_number_matcher(chnum));
		if (i == channels_.end()) {
			throw std::runtime_error("Invalid channel!");
		}
		return *i;
	}

	void remove_channel(const unsigned int chnum)
	{
		using std::find_if;
		channel_container::iterator i =
			find_if(channels_.begin(), channels_.end(),
					detail::channel_number_matcher(chnum));
		if (i == channels_.end()) {
			throw std::runtime_error("Invalid channel!");
		}
		channels_.erase(i);
	}

	/// \return the used message number
	unsigned int send_message(const message &msg, channel& chan)
	{
		using std::vector;
		using std::back_inserter;

		vector<frame> frames;
		const unsigned int message_number = make_frames(msg, chan, back_inserter(frames));
		assert(!frames.empty());
		transport_.send_frames(id_, frames.begin(), frames.end());
		return message_number;
	}

	unsigned int send_tuning_message(const message &msg)
	{
		return send_message(msg, chman_.tuning_channel());
	}

	void close_transport(const boost::system::error_code &error)
	{
		if (error) {
			throw boost::system::system_error(error);
		}
		transport_.shutdown_connection(id_);
	}
};     // class basic_session

template <typename TransportServiceT>
void shutdown_session(basic_session<TransportServiceT> &session)
{
	using boost::bind;
	typedef basic_session<TransportServiceT> session_type;
	session.async_close_channel(0, reply_code::success,
								bind(&session_type::close_transport,
									 &session, _1));
}

}      // namespace beep
#endif // BEEP_SESSION_HEAD
