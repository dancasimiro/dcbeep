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
#include "channel-manager.hpp"

namespace beep {
namespace detail {

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

using std::make_pair;

/// \return a pair<message, bool> 
///         The message is the response.
///         The bool encodes whether the session should be shutdown (call transport_.shutdown_connection(id_))
class tuning_message_visitor : public boost::static_visitor<std::pair<message, bool> > {
public:
	typedef std::pair<message, bool> result_type;
	tuning_message_visitor (channel_manager &chman) : manager_(chman) { }

	result_type operator()(const cmp::start_message &msg) const
	{
		// send "OK" message before I execute the profile handler and
		// _possibly_ send channel data.
		const cmp::protocol_node response = manager_.accept_start(msg);
		return make_pair(cmp::generate(response), false);
	}

	result_type operator()(const cmp::close_message &msg) const
	{
		std::pair<bool, cmp::protocol_node> result =
			manager_.peer_requested_channel_close(msg);
		return make_pair(cmp::generate(result.second), result.first);
	}

	result_type operator()(const cmp::ok_message &) const
	{
		cmp::error_message response;
		response.code = reply_code::parameter_invalid;
		response.diagnostic = "This OK message is not expected.";
		return make_pair(cmp::generate(response), false);
	}

	result_type operator()(const cmp::greeting_message &) const
	{
		cmp::error_message response;
		response.code = reply_code::parameter_invalid;
		response.diagnostic = "The greeting message should arrive in a 'RPY' frame.";
		return make_pair(cmp::generate(response), false);
	}

	result_type operator()(const cmp::error_message &) const
	{
		cmp::error_message response;
		response.code = reply_code::parameter_invalid;
		response.diagnostic = "An error message should arrive in an 'ERR' frame.";
		return make_pair(cmp::generate(response), false);
	}

	result_type operator()(const cmp::profile_element &) const
	{
		cmp::error_message response;
		response.code = reply_code::parameter_invalid;
		response.diagnostic = "The profile element should arrive in a 'RPY' frame.";
		return make_pair(cmp::generate(response), false);
	}
private:
	channel_manager &manager_;
};     // tuning_message_visitor

enum reply_action {
	session_start_was_requested,
	session_start_was_accepted,
	invalid_message_received,
	channel_close_was_accepted,
};

class tuning_reply_visitor : public boost::static_visitor<reply_action> {
public:
	tuning_reply_visitor (channel_manager &chman) : manager_(chman) { }

	reply_action operator()(const cmp::greeting_message &greeting) const
	{
		for (std::vector<std::string>::const_iterator i = greeting.profile_uris.begin();
			 i != greeting.profile_uris.end(); ++i) {
			manager_.install_profile(*i);
		}
		return session_start_was_requested;
	}

	reply_action operator()(const cmp::start_message &) const
	{
		return invalid_message_received;
	}

	reply_action operator()(const cmp::close_message &) const
	{
		return invalid_message_received;
	}

	reply_action operator()(const cmp::ok_message &) const
	{
		return channel_close_was_accepted;
	}

	reply_action operator()(const cmp::error_message &) const
	{
		return invalid_message_received;
	}

	reply_action operator()(const cmp::profile_element &) const
	{
		return session_start_was_accepted;
	}
private:
	channel_manager &manager_;
};     // tuning_reply_visitor

class tuning_error_visitor : public boost::static_visitor<boost::system::error_code> {
public:
	boost::system::error_code operator()(const cmp::greeting_message &) const
	{
		throw std::runtime_error("The 'ERR' frame should not contain a greeting message.");
		return boost::system::error_code(reply_code::transaction_failed, beep::beep_category);
	}

	boost::system::error_code operator()(const cmp::start_message &) const
	{
		throw std::runtime_error("The 'ERR' frame should not contain a start message.");
		return boost::system::error_code(reply_code::transaction_failed, beep::beep_category);
	}

	boost::system::error_code operator()(const cmp::close_message &) const
	{
		throw std::runtime_error("The 'ERR' frame should not contain a close message.");
		return boost::system::error_code(reply_code::transaction_failed, beep::beep_category);
	}

	boost::system::error_code operator()(const cmp::ok_message &) const
	{
		throw std::runtime_error("The 'ERR' frame should not contain an OK message.");
		return boost::system::error_code(reply_code::transaction_failed, beep::beep_category);
	}

	boost::system::error_code operator()(const cmp::error_message &msg) const
	{
		/// \todo Check that msg.code is a valid reply code
		return boost::system::error_code(msg.code, beep::beep_category);
	}

	boost::system::error_code operator()(const cmp::profile_element &) const
	{
		throw std::runtime_error("The 'ERR' frame should not contain a profile element.");
		return boost::system::error_code(reply_code::transaction_failed, beep::beep_category);
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

	typedef typename transport_service::identifier identifier;

	typedef boost::signals2::connection      signal_connection;
	typedef boost::signals2::signal<void (const boost::system::error_code&)> session_signal_t;

	template <typename U> friend void shutdown_session(basic_session<U>&);

	basic_session(transport_service_reference ts)
		: transport_(ts)
		, id_()
		, frmsig_()
		, chman_()
		, mcompiler_()
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
		, mcompiler_()
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
		std::pair<unsigned int, cmp::protocol_node> result =
			chman_.start_channel(transport_service::get_role(), strm.str(), profile_uri);
		const unsigned int channel_number = result.first;
		if (channel_number > 0) {
			message start = cmp::generate(result.second);
			try {
				// send_tuning_message updates message_number inside start message
				send_tuning_message(start);
				const unsigned int msgno = start.get_channel().get_message_number();
				tuning_handler_.add(msgno, bind(handler, _1, channel_number, profile_uri));
			} catch (const std::exception &ex) {
				chman_.close_channel(channel_number, reply_code::requested_action_aborted);
				return 0u;
			}
		}
		return channel_number;
	}

	template <class Handler>
	void async_close_channel(const unsigned int channel,
							 const reply_code::rc_enum rc, Handler handler)
	{
		const cmp::protocol_node request = chman_.close_channel(channel, rc);
		message close = cmp::generate(request);
		// send_tuning_message updates the message number inside of 'close'
		send_tuning_message(close);
		const unsigned int msgno = close.get_channel().get_message_number();
		tuning_handler_.add(msgno, bind(handler, _1, channel));
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

	void send(const unsigned int channel, message &msg)
	{
		chman_.prepare_message_for_channel(channel, msg);
		send_message(msg);
	}
private:
	typedef typename transport_service::signal_connection signal_connection_t;
	typedef boost::system::error_code error_code;
	typedef boost::function<void (const error_code&, unsigned, bool, const message&)> function_type;

	transport_service_reference   transport_;
	identifier                    id_;
	signal_connection_t           frmsig_;

	channel_manager               chman_;
	message_compiler              mcompiler_;

	detail::handler_tuning_events tuning_handler_;
	detail::handler_user_events   user_handler_;

	session_signal_t              session_signal_;

	void handle_frame(const boost::system::error_code &error, const frame &frm)
	{
		if (!error) {
			try {
				message msg;
				if (mcompiler_(frm, msg)) {
					if (msg.get_channel().get_number() == detail::tuning_channel_number()) {
						handle_tuning_message(msg);
					} else {
						handle_user_message(msg);
					}
				}
			} catch (const std::exception &ex) {
				/// \todo handle the error condition!
				/// \todo Check if this is the right error condition.
				///       Maybe I should close the channel?
				cmp::error_message err;
				err.code = reply_code::requested_action_aborted;
				err.diagnostic = ex.what();
				message msg = cmp::generate(err);
				send_tuning_message(msg);
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
				std::pair<message, bool> response =
					apply_visitor(detail::tuning_message_visitor(chman_), my_node);
				send_tuning_message(response.first);
				chman_.invoke_pending_channel_notifications();
				if (response.second) {
					transport_.shutdown_connection(id_);
				}
			}
			break;
		case RPY:
			switch (apply_visitor(detail::tuning_reply_visitor(chman_), my_node)) {
			case detail::session_start_was_requested:
				session_signal_(boost::system::error_code());
				break;
			case detail::session_start_was_accepted:
				tuning_handler_.execute(msg.get_channel().get_message_number(), boost::system::error_code());
				break;
			case detail::channel_close_was_accepted:
				// if msg.channel == tuning_channel
				//session_signal_(boost::system::error_code());
				// else
				tuning_handler_.execute(msg.get_channel().get_message_number(), boost::system::error_code());
				break;
			case detail::invalid_message_received:
				{
					cmp::error_message response;
					response.code = reply_code::parameter_invalid;
					response.diagnostic = "An unexpected message type was found in RPY frame.";
					message my_message = cmp::generate(response);
					send_tuning_message(my_message);
				}
				break;
			default:
				assert(false);
				throw std::runtime_error("Invalid reply action!");
				break;
			}
			break;
		case ERR:
			{
				const error_code ec = apply_visitor(detail::tuning_error_visitor(), my_node);
				/// \todo log the error description (error.what())
				tuning_handler_.execute(msg.get_channel().get_message_number(), ec);
			}
			break;
		case ANS:
		case NUL:
		default:
			assert(false);
			throw std::runtime_error("invalid message type in tuning handler...");
		}
	}

	void handle_user_message(const message &msg)
	{
		boost::system::error_code error;
		user_handler_.execute(msg.get_channel().get_number(), error, msg);
	}

	void start()
	{
		const cmp::protocol_node request = chman_.get_greeting_message();
		message greeting = cmp::generate(request);
		send_tuning_message(greeting);
	}

	void send_message(message &msg)
	{
		using std::vector;
		using std::back_inserter;

		vector<frame> frames;
		make_frames(msg, back_inserter(frames));
		assert(!frames.empty());
		transport_.send_frames(id_, frames.begin(), frames.end());
	}

	void send_tuning_message(message &msg)
	{
		send(detail::tuning_channel_number(), msg);
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
	typedef basic_session<TransportServiceT> session_type;
	if (session.chman_.channel_in_use(0)) {
		session.async_close_channel(0, reply_code::success,
					    bind(&session_type::close_transport, &session, _1));
	}
}

}      // namespace beep
#endif // BEEP_SESSION_HEAD
