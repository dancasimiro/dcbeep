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
};
#if 0
inline std::istream&
decode_start_channel(istream &strm, int &channel)
{
	if (strm) {
		string line;
		getline(strm, line, '\'');
		strm >> channel;
		getline(strm, line); // eat the rest of this line
	}
	return strm;
}

inline std::istream&
decode_start_profile(std::istream &strm, string &uri)
{
	if (strm) {
		string xml;
		if (getline(strm, xml)) {
			const string::size_type startIdx = xml.find('\'');
			const string::size_type endIdx = xml.rfind('\'');
			if (startIdx != string::npos && endIdx != string::npos) {
				const size_t len = endIdx - startIdx - 1;
				uri = xml.substr(startIdx + 1, len);
			} else {
				strm.setstate(std::ios::badbit);
			}
		}
	}
	return strm;
}

template <class T>
reply_code
start_channel(T psession, channel &aChannel, const string &init)
{
	return psession ? psession->initialize(aChannel, init) : service_not_available;
}

template <class T>
reply_code
stop_channel(T psession, const int chNum)
{
	return psession ? psession->remove_channel(chNum) : service_not_available;
}

template <class T>
channel extract_channel_from_pair(T value)
{
	return value.second.first;
}

template <class TransportLayer>
class session_impl
	: public enable_shared_from_this<session_impl<TransportLayer> >
{
public:
	typedef TransportLayer                                  transport_layer;
	typedef basic_session<transport_layer>                  session_type;
	typedef session_type*                                   session_pointer;
	typedef session_impl<transport_layer>                   full_type;
	typedef enable_shared_from_this<full_type>              base_type;
	typedef typename transport_layer::connection_type       stream_type;
	typedef vector<char>                                    buffer_type;
	typedef channel_management_profile                      tuner_profile;
	typedef shared_ptr<full_type>                           pointer;

	session_impl(transport_layer &transport)
		: base_type()
		, psession_(NULL)
		, ready_(false)
		, tuner_()
		, tunebuf_()
		, tuneprof_()
		, connection_(transport.lowest_layer())
	{
		tuner_.set_number(0);
		tunebuf_.resize(4096);
	}

	virtual ~session_impl()
	{
	}

	void set_session(session_pointer p) { psession_ = p; }
	stream_type &connection() { return connection_; }
	const stream_type &connection() const { return connection_; }

	void start()
	{
		message msg;
		if (tuneprof_.initialize(msg)) {
			connection_.start();
			connection_.send(tuner_, msg,
							 bind(&session_impl::sent_greeting,
								  this->shared_from_this(), _1, _2));
		}
	}

	template <class Handler>
	void stop(Handler handler)
	{
		ready_ = false;
		channel oldTuner = tuner_;
		channel newTuner;
		newTuner.set_number(0);
		tuner_ = newTuner;
		this->close_channel(oldTuner, handler);
	}

	template <typename MutableBuffer, class Handler>
	void async_read(const channel &channel, MutableBuffer aBuffer, Handler aHandler)
	{
		read_helper<Handler> help(this->shared_from_this(), channel, aHandler);
		connection_.read(channel, aBuffer, help);
	}

	template <typename ConstBuffer, class Handler>
	void async_send(channel &channel, ConstBuffer aBuffer, Handler aHandler)
	{
		write_helper<Handler> help(this->shared_from_this(), channel, aHandler);
		connection_.send(channel, aBuffer, help);
	}

	tuner_profile &profile() { return tuneprof_; }

	void standup(const channel &channel)
	{
		if (ready_) {
			// sending a new channel start message must be delayed until the
			// supported profiles are negogiated. The registration process must
			// be completed.
			this->do_standup(channel);
		}
	}

	template <class Handler>
	void standdown(const channel &chan, Handler handler)
	{
		if (ready_) {
			this->close_channel(chan, handler);
		} else if (psession_) {
			handler(requested_action_not_accepted, *psession_, chan);
		}
	}
private:
	session_pointer           psession_;
	bool                      ready_;
	channel                   tuner_;
	buffer_type               tunebuf_;
	tuner_profile             tuneprof_;
	stream_type               connection_;

	void
	sent_greeting(const boost::system::error_code &error,
				  size_t bytes_transferred)
	{
		if (!error || error == asio::error::message_size) {
			// read the greeting message from my peer
			connection_.read(tuner_, buffer(tunebuf_),
							 bind(&session_impl::negogiate_session,
								  this->shared_from_this(),
								  asio::placeholders::error,
								  asio::placeholders::bytes_transferred,
								  _3));
		} else {
			/// \todo send the error up the chain, or try again...
			ostringstream strm;
			strm << "there was an error sending the beep greeting ("
				 << error.message() << ").";
			//throw runtime_error(strm.str());
		}
	}

	void
	negogiate_session(const boost::system::error_code &error,
					  size_t bytes_transferred, const frame::frame_type ft)
	{
		if (!error || error == asio::error::message_size) {
			/// the tunebuf_ should be filled with a greeting message
			/// \todo actually negogiate
			ready_ = true;

			/// for now, just read more data for channel 0
			connection_.read(tuner_, buffer(tunebuf_),
							 bind(&session_impl::on_tuning_message,
								  this->shared_from_this(),
								  asio::placeholders::error,
								  asio::placeholders::bytes_transferred,
								  _3));

			// send "start" messages for any channels that were added
			// before the negotiation finished.
			if (psession_) {
				typedef list<channel> container;
				container channels;
				psession_->copy_channels(back_insert_iterator<container>(channels));
				for_each(channels.begin(), channels.end(),
						 bind(&session_impl::do_standup,
							  this->shared_from_this(),
							  _1));
			}
		} else {
			cerr << "Failed to negogiate the session info: " << error.message() << endl;
		}
	}

	void
	on_tuning_message(const boost::system::error_code &error,
					  size_t bytes_transferred,
					  const frame::frame_type ft)
	{
		if (!error || error == asio::error::message_size) {
			/// \todo install an XML parser here...
			typedef buffer_type::const_iterator const_iterator;
			const_iterator first(tunebuf_.begin()), last(first);
			advance(last, bytes_transferred);
			string myString(first, last);

			// if start message
			reply_code status = general_syntax_error;
			if(myString.find("start") != string::npos) {
				istringstream strm(myString);
				int channel = -1;
				string profile_uri;
				if (decode_start_channel(strm, channel) &&
					decode_start_profile(strm, profile_uri)) {
					status = this->setup_new_channel(channel, profile_uri);
				}
			} else if (myString.find("close") != string::npos) {
				istringstream strm(myString);
				int channel = -1;
				if (decode_start_channel(strm, channel)) {
					if (channel > 0) {
						status = this->shut_down_channel(channel);
					} else {
						status = this->shut_down_session();
					}
				}
			}
			connection_.read(tuner_,
							 buffer(tunebuf_),
							 bind(&session_impl::on_tuning_message,
								  this->shared_from_this(),
								  asio::placeholders::error,
								  asio::placeholders::bytes_transferred,
								  _3));
		} else {
			/// \todo the error should be passed to the class holder...
			cerr << "error receiving data on the tuning channel: "
				 << error.message() << endl;
			ready_ = false;
		}
	}

	void
	send_tuning_reply(const reply_code rc, const string &uri)
	{
		// storage is buffered inside the connection object.
		// It does not need to live past the connection send call.
		ostringstream strm;
		string storage;
		message msg;
		if (rc != success) {
			tuneprof_.encode_error(rc, strm);
			storage = strm.str();
			tuneprof_.make_message(frame::err, storage, msg);
		} else {
			tuneprof_.accept_profile(uri, strm);
			storage = strm.str();
			tuneprof_.make_message(frame::rpy, storage, msg);
		}
		connection_.send(tuner_, msg,
						 bind(&session_impl::on_sent_tuning_reply,
							  this->shared_from_this(),
							  asio::placeholders::error,
							  asio::placeholders::bytes_transferred));
	}

	void
	on_sent_tuning_reply(const boost::system::error_code &error,
						 size_t bytes_transferred)
	{
		if (error && error != boost::asio::error::message_size) {
			ostringstream strm;
			strm << "Failed to send the tuning reply: " << error.message();
			//throw runtime_error(strm.str());
		}
	}

	reply_code
	setup_new_channel(const int chNum, const string &profileURI)
	{
		channel myChannel;
		myChannel.set_number(chNum);
		myChannel.set_profile(profileURI);

		/// \todo read the initialization message from the start message
		string init;
		const reply_code status = start_channel(psession_, myChannel, init);
		this->send_tuning_reply(status, profileURI);
		return status;
	}

	reply_code
	shut_down_channel(const int chNum)
	{
		const reply_code status = stop_channel(psession_, chNum);
		this->send_channel_close_reply(status);
		return status;
	}

	reply_code
	shut_down_session()
	{
		reply_code status = success;
		this->send_channel_close_reply(status);
		connection_.stop();
		return status;
	}

	void
	send_channel_close_reply(const reply_code rc)
	{
		// storage is buffered inside the connection object.
		// It does not need to live past the connection send call.
		ostringstream strm;
		string storage;
		message msg;
		if (rc != success) {
			tuneprof_.encode_error(rc, strm);
			storage = strm.str();
			tuneprof_.make_message(frame::err, storage, msg);
		} else {
			tuneprof_.acknowledge(strm);
			storage = strm.str();
			tuneprof_.make_message(frame::rpy, storage, msg);
		}
		connection_.send(tuner_, msg,
						 bind(&session_impl::on_sent_tuning_reply,
							  this->shared_from_this(),
							  asio::placeholders::error,
							  asio::placeholders::bytes_transferred));
	}

	void
	do_standup(const channel &chan)
	{
		ostringstream encstrm;
		if (tuneprof_.add_channel(chan, encstrm)) {
			const string content(encstrm.str());
			message msg;
			tuneprof_.make_message(frame::msg, content, msg);
			connection_.send(tuner_, msg,
							 bind(&session_impl::on_sent_channel_start,
								  this->shared_from_this(),
								  asio::placeholders::error,
								  asio::placeholders::bytes_transferred,
								  chan));
		} else {
			psession_->invoke_handler(chan, service_not_available);
		}
	}

	void
	on_sent_channel_start(const boost::system::error_code &error,
						  size_t bytes_transferred,
						  const channel &chan)
	{
		reply_code status = success;
		if (error && error != asio::error::message_size) {
			status = requested_action_aborted;
		}
		if (psession_) {
			psession_->invoke_handler(chan, success);
		}
	}

	template <class Handler>
	void
	close_channel(const channel &aChannel, Handler handler)
	{
		ostringstream encstrm;
		if (connection_.lowest_layer().is_open() &&
			tuneprof_.remove_channel(aChannel, encstrm)) {
			message msg;
			const string content(encstrm.str());
			tuneprof_.make_message(frame::msg, content, msg);
			channel_closer<Handler> helper(this->shared_from_this(),
										   aChannel, handler);

			connection_.send(tuner_, msg, helper);
		} else if (psession_) {
			handler(requested_action_aborted, *psession_, aChannel);
		}
	}

	template <class Handler>
	struct channel_closer {
		typedef void                    result_type;

		pointer               theSession;
		channel               theChannel;
		Handler               theHandler;

		channel_closer(pointer aSession, const channel &c, Handler h)
			: theSession(aSession)
			, theChannel(c)
			, theHandler(h)
		{
		}

		void operator()(const boost::system::error_code &error,
						size_t bytes_transferred)
		{
			reply_code status = success;
			if (error && error != boost::asio::error::message_size) {
				status = requested_action_aborted;
			}
			if (theChannel.number() == 0) {
				theSession->connection_.stop();
			}
			if (session_pointer p = theSession->psession_) {
				theHandler(status, *p, theChannel);
			}
		}
	};

	template <class Handler>
	struct read_helper {
		pointer               theSession;
		channel               theChannel;
		Handler               theHandler;

		read_helper(pointer aSession, const channel &c, Handler h)
			: theSession(aSession)
			, theChannel(c)
			, theHandler(h)
		{
		}

		read_helper(const read_helper &src)
			: theSession(src.theSession)
			, theChannel(src.theChannel)
			, theHandler(src.theHandler)
		{
		}

		void operator()(const boost::system::error_code &error,
						const std::size_t bytes_transferred,
						const frame::frame_type ft) const
		{
			reply_code status = success;
			if (error && error != boost::asio::error::message_size) {
				status = requested_action_aborted;
			}
			if (session_pointer p = theSession->psession_) {
				theHandler(status, *p, theChannel, bytes_transferred);
			}
		}
	};

	template <class Handler>
	struct write_helper {
		typedef void                    result_type;

		pointer              theSession;
		channel              theChannel;
		Handler              theHandler;

		write_helper(pointer aSession, const channel &c, Handler h)
			: theSession(aSession)
			, theChannel(c)
			, theHandler(h)
		{
		}

		void operator()(const boost::system::error_code &error,
						const std::size_t bytes_transferred)
		{
			reply_code status = success;
			if (error && error != boost::asio::error::message_size) {
				status = requested_action_aborted;
			}
			if (session_pointer p = theSession->psession_) {
				theHandler(status, *p, theChannel, bytes_transferred);
			}
		}
	};
};     // class session_impl
#endif
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
	typedef TransportServiceT                     transport_service;
	typedef transport_service&                    transport_service_reference;
	typedef basic_session<transport_service>      base_type;
#if 0
	/// this is a leaked implementation detail. do not use it.
	reply_code initialize(channel &aChannel, const string &init)
	{
		typedef typename profile_container::iterator iterator;
		iterator i = profiles_.find(aChannel.profile());
		return (i != profiles_.end() ? 
				i->second(*this, aChannel, init) : requested_action_aborted);
	}

	reply_code remove_channel(const int chNum)
	{
		return channels_.erase(chNum) > 0 ? success : requested_action_aborted;
	}

	template <typename InputIterator>
	void copy_channels(InputIterator first)
	{
		transform(channels_.begin(), channels_.end(), first,
				  detail::extract_channel_from_pair<typename channel_container::value_type>);
	}

	void invoke_handler(const channel &aChannel, const reply_code status)
	{
		typedef typename channel_container::const_iterator const_iterator;
		const const_iterator i = channels_.find(aChannel.number());
		if (i != channels_.end()) {
			i->second.second(status, *this, i->second.first);
		}
	}

	basic_session(transport_layer_reference transport)
		: nextchan_(1)
		, profiles_()
		, channels_()
		, pimpl_(new impl_type(transport))
	{
		pimpl_->set_session(this);
	}
#endif
	typedef std::size_t size_type;

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
		using std::vector;
		using std::back_inserter;

		message greeting;
		chman_.get_greeting_message(profiles_.begin(), profiles_.end(), greeting);

		vector<frame> frames;
		make_frames(greeting,
					chman_.get_tuning_channel(),
					back_inserter(frames));
		transport_.send_frames(frames.begin(), frames.end());
	}
#if 0
	virtual ~basic_session()
	{
		pimpl_->set_session(NULL);
	}

	connection_layer_type &connection_layer()
	{
		return pimpl_->connection().lowest_layer();
	}

	/// \brief Associate a new profile with the session
	///
	/// The session adds this profile URI to the list of advertised profiles.
	/// The handler is invoked whenever a peer initiates a new channel with this
	/// profile.
	///
	/// \param profile A concrete profile object.
	/// \param handler A handler function or object with the following signature
	///
	template <class Handler>
	void install(const string &profile, Handler handler)
	{
		profiles_.insert(make_pair(profile, handler));
		pimpl_->profile().add_profile(profile);
	}

	template <class FwdIterator>
	void install(FwdIterator first, const FwdIterator last)
	{
		profiles_.insert(first, last);
		for (; first != last; ++first) {
			pimpl_->profile().add_profile(first->first);
		}
	}

	template <class Handler>
	int add_channel(const string &profile, Handler handler)
	{
		channel info;
		info.set_number(nextchan_);
		nextchan_ += 2;
		info.set_profile(profile);
		return this->add_channel(info, handler);
	}

	template <class Handler>
	int add_channel(const channel &info, Handler handler)
	{
		typedef typename channel_container::iterator iterator;
		typedef pair<iterator, bool> result_type;

		int chNum = info.number();
		channel_pair myPair(make_pair(info, handler));

		const result_type result = channels_.insert(make_pair(chNum, myPair));
		if (result.second) {
			pimpl_->standup(info);
		} else {
			chNum = -1;
		}
		return chNum;
	}

	template <class Handler>
	bool remove_channel(const channel &chan, Handler handler)
	{
		typedef typename channel_container::size_type size_type;
		size_type n = channels_.erase(chan.number());
		if (n > 0) {
			pimpl_->standdown(chan, handler);
		}
		return (n > 0);
	}

	void start()
	{
		pimpl_->start();
	}

	template <class Handler>
	void stop(Handler handler)
	{
		profiles_.clear();
		channels_.clear();
		nextchan_ %= 2; // reset next chan to 0 or 1
		if (nextchan_ == 0) {
			nextchan_ = 2;
		}
		pimpl_->stop(handler);
	}

	template <typename MutableBuffer, class Handler>
	void async_read(const channel &channel, MutableBuffer aBuffer, Handler aHandler)
	{
		pimpl_->async_read(channel, aBuffer, aHandler);
	}

	template <typename ConstBuffer, class Handler>
	void async_send(channel &channel, ConstBuffer aBuffer, Handler aHandler)
	{
		pimpl_->async_send(channel, aBuffer, aHandler);
	}
	// for now, I am hardcoding the handlers with boost::function typedefs.
	// I can extend this to be more generic later
	typedef function<reply_code (basic_session&, channel&, string)>   profile_handler;
	typedef function<void (reply_code, basic_session&, const channel&)>     channel_handler;

	typedef map<string,  profile_handler>                   profile_container;

	typedef pair<channel, channel_handler>                  channel_pair;
	typedef map<int, channel_pair>                          channel_container;
#endif
private:
#if 0
	typedef channel_management_profile                      tuner_profile;
	typedef detail::session_impl<transport_layer>           impl_type;
	typedef typename impl_type::pointer                     pimpl_type;
#endif
	typedef typename transport_service::signal_connection   signal_connection_t;
	typedef std::map<std::string, profile>                  profile_container;
	transport_service_reference   transport_;
	identifier                    id_;
	signal_connection_t           netchng_;
	signal_connection_t           frmsig_;

	channel_manager               chman_;
	detail::handler_tuning_events handler_;
	profile_container             profiles_;
	//unsigned int              nextchan_;
	//profile_container         profiles_;
	//channel_container         channels_;
	//pimpl_type                pimpl_;

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
