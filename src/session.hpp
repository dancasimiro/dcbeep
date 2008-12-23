/// \file  session.hpp
/// \brief BEEP session built on Boost ASIO
///
/// UNCLASSIFIED
#ifndef BEEP_SESSION_HEAD
#define BEEP_SESSION_HEAD 1
namespace beep {

enum role {
	initiating_role,
	listening_role,
};

enum reply_code {
	success                                      = 200,
	service_not_available                        = 421,
	requested_action_not_taken                   = 450, ///< \note e.g., lock already in use
	requested_action_aborted                     = 451, ///< \note e.g., local error in processing
	temporary_authentication_failure             = 454,
	general_syntax_error                         = 500, ///< \note e.g., poorly-formed XML
	syntax_error_in_parameters                   = 501, ///< \note e.g., non-valid XML
	parameter_not_implemented                    = 504,
	authentication_required                      = 530,
	authentication_mechanism_insufficient        = 534, ///< \note e.g., too weak, sequence exhausted, etc.
	authentication_failure                       = 535,
	action_not_authorized_for_user               = 537,
	authentication_mechanism_requires_encryption = 538,
	requested_action_not_accepted                = 550, ///< \note e.g., no requested profiles are acceptable
	parameter_invalid                            = 553,
	transaction_failed                           = 554, ///< \note e.g., policy violation
};


template <class U> class basic_session;

namespace detail {

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
			throw runtime_error(strm.str());
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
						 bind(&session_impl::standup_channel,
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
			throw runtime_error(strm.str());
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
	standup_channel(const channel &chan)
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
		if (tuneprof_.remove_channel(aChannel, encstrm)) {
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
			} else if (theChannel.number() == 0) {
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

}      // namespace detail

/// \brief BEEP session management
///
/// \note Each session has an implicit tuning channel that performs
///       initialization.
template <class TransportLayer>
class basic_session
	: private noncopyable
{
public:
	typedef TransportLayer                        transport_layer;
	typedef transport_layer&                      transport_layer_reference;
	typedef basic_session<transport_layer>        base_type;
	typedef typename transport_layer::connection_type  connection_type;
	typedef typename connection_type::lowest_layer_type     connection_layer_type;

	template<class SessionType> friend class basic_listener;
	template<class SessionType> friend class basic_initiator;
	//template<class T> friend reply_code start_channel(T, channel&, const string&);

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
		, ready_(false)
		, connection_(transport.lowest_layer())
		, profiles_()
		, channels_()
		, tuner_()
		, tuneprof_()
		, tunebuf_()
		, name_("unnamed session")
		, pimpl_(new impl_type(transport))
	{
		pimpl_->set_session(this);
		tuner_.set_number(0);
		tunebuf_.resize(4096);
	}

	basic_session(transport_layer_reference transport, role r)
		: nextchan_(r == initiating_role ? 1 : 2)
		, ready_(false)
		, connection_(transport.lowest_layer())
		, profiles_()
		, channels_()
		, tuner_()
		, tuneprof_()
		, tunebuf_()
		, name_("unnamed session")
		, pimpl_(new impl_type(transport))
	{
		pimpl_->set_session(this);
		tuner_.set_number(0);
		tunebuf_.resize(4096);
	}

	virtual ~basic_session()
	{
		pimpl_->set_session(NULL);
	}

	connection_layer_type &connection_layer()
	{
		return pimpl_->connection().lowest_layer();
	}

	const string &name() const { return name_; }
	void set_name(const string &n) { name_ = n; }

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
		tuneprof_.add_profile(profile);
	}

	template <class FwdIterator>
	void install(FwdIterator first, const FwdIterator last)
	{
		profiles_.insert(first, last);
		for (; first != last; ++first) {
			tuneprof_.add_profile(first->first);
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
			if (ready_) {
				// sending a new channel start message must be delayed until the
				// supported profiles are negogiated. The registration process must
				// be completed.
#if 0
				standup_channel(*result.first);
#endif
			}
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
			if (ready_) {
#if 0
				close_channel(chan, handler);
#endif
			} else {
				handler(requested_action_not_accepted, *this, chan);
			}
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
private:
	typedef channel_management_profile                      tuner_profile;
	typedef detail::session_impl<transport_layer>           impl_type;
	typedef typename impl_type::pointer                     pimpl_type;

	unsigned int              nextchan_;
	bool                      ready_;     // registration is complete.
	connection_type           connection_;
	profile_container         profiles_;
	channel_container         channels_;
	channel                   tuner_;
	tuner_profile             tuneprof_;
	vector<char>              tunebuf_;
	string                    name_;    // only here for debugging
	pimpl_type                pimpl_;
};     // class basic_session

}      // namespace beep
#endif // BEEP_SESSION_HEAD
