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

/// \brief BEEP session management
///
/// \note Each session has an implicit tuning channel that performs
///       initialization.
template <class TransportLayer>
class basic_session :
		private noncopyable
{
public:
	typedef TransportLayer                        transport_layer;
	typedef transport_layer&                      transport_layer_reference;
	typedef basic_session<transport_layer>        base_type;
	typedef typename transport_layer::connection_type  connection_type;
	typedef connection_type&                      connection_reference;
	typedef basic_channel<base_type>              channel_type;
	typedef shared_ptr<profile>                   profile_pointer;
	typedef function<void ()>                     channel_accepted_callback;
	typedef boost::system::error_code             error_type;
	typedef function<void (error_type)>           error_callback;

	basic_session(transport_layer_reference transport)
		: transport_(transport)
		, connection_(transport_.lowest_layer())
		, channels_()
		, profiles_()
		, role_(initiating_role)
		, nextchan_(1)
		, tunebuf_()
		, tuneprof_(new channel_management_profile)
		, ready_(false)
		, delegate_(NULL)
		, chcb_()
		, error_handler_()
	{
		tunebuf_.resize(4096);
		setup_tuning_channel();
	}

	basic_session(transport_layer_reference transport, role r)
		: transport_(transport)
		, connection_(transport_.lowest_layer())
		, channels_()
		, profiles_()
		, role_(r)
		, nextchan_(r == initiating_role ? 1 : 2)
		, tunebuf_()
		, tuneprof_(new channel_management_profile)
		, ready_(false)
		, delegate_(NULL)
		, chcb_()
		, error_handler_()
	{
		tunebuf_.resize(4096);
		setup_tuning_channel();
	}

	struct delegate {
		virtual void channel_is_ready(const channel_type &channel) = 0;
	};

	io_service &lowest_layer() { return transport_.lowest_layer(); }
	connection_reference connection() { return connection_; }
	void set_delegate(delegate * const del) { delegate_ = del; }

	void start()
	{
		cout << "session::start" << endl;
		connection_.start();

		message msg;
		if (tuning_channel().get_profile()->initialize(msg)) {
			connection_.async_send(tuning_channel(), msg,
								   bind(&basic_session::sent_channel_greeting,
										this, _1, _2));
		} else {
			cerr << "Failed to transmit the greeting. :-(\n";
			assert(false);
		}
	}

	unsigned int next_channel_number() const { return nextchan_; }

	template <typename Handler>
	void async_add(channel_type &channel, Handler handler)
	{
		this->track_channel(channel);
		chcb_.insert(make_pair(channel.number(), handler));

		// sending a new channel start message must be delayed until the
		// supported profiles are negogiated. The registration process must
		// be completed.

		// if the registration is complete...
		// get management syntax
		if (ready_) { // if the registration process is complete
			this->standup_channel(channel);
		}
	}

	void install(profile_pointer pp)
	{
		profiles_.push_back(pp);
	}

	void close()
	{
		connection().lowest_layer().close();
	}
private:
	typedef vector<channel_type>                            channels_container;
	typedef list<profile_pointer>                           profiles_container;
	typedef shared_ptr<channel_management_profile>          tuning_profile;
	typedef map<int, channel_accepted_callback>             cb_container;

	transport_layer_reference transport_;
	connection_type           connection_;
	channels_container        channels_;
	profiles_container        profiles_;
	role                      role_;
	unsigned int              nextchan_;
	vector<char>              tunebuf_;
	tuning_profile            tuneprof_;
	bool                      ready_;     // registration is complete.
	delegate                  *delegate_;
	cb_container              chcb_;

	void
	on_channel_management(const boost::system::error_code &error,
						  size_t bytes_transferred)
	{
		if (error) {
			cout << "the session had a read error!" << endl;
			error_handler_(error);
			return;
		}
		/// \todo install an XML parser here...
		cout << "on_channel_management recveived " << bytes_transferred
			 << " bytes:\n";
		string myString(tunebuf_.begin(), tunebuf_.begin() + bytes_transferred);

		// if start message
		if(myString.find("start") != string::npos) {
			string garbage;
			istringstream strm(myString);
			getline(strm, garbage, '\'');
			// need a profile...
			int channel = -1;
			strm >> channel;
			string profile_uri;
			this->setup_new_channel(channel, profile_uri);
		}
		connection_.async_read(this->tuning_channel(),
							   buffer(tunebuf_),
							   bind(&basic_session::on_channel_management,
									this,
									asio::placeholders::error,
									asio::placeholders::bytes_transferred));
	}

	void
	sent_channel_greeting(const boost::system::error_code &error,
						  size_t bytes_transferred)
	{
		cout << "session::sent_channel_greeting" << endl;
		/// \todo negogiate the connection
		/// for example, authorize user or set up encryption
		connection_.async_read(this->tuning_channel(), buffer(tunebuf_),
							   bind(&basic_session::negogiate_session,
									this,
									asio::placeholders::error,
									asio::placeholders::bytes_transferred));
	}

	void
	negogiate_session(const boost::system::error_code &error,
					  size_t bytes_transferred)
	{
		cout << "session::negogiate_session" << endl;
		if (!error || error == asio::error::message_size) {
			/// \todo actually negogiate
			ready_ = true;

			// send "start" messages for any channels that were added
			// before the negotiation finished.
			// skip over the tuning channel (channel 0)
			for_each(channels_.begin() + 1, channels_.end(),
					 bind(&basic_session::standup_channel, this, _1));

			/// for now, just read more data for channel 0
			this->on_channel_management(error, bytes_transferred);
		}
	}

	void
	setup_new_channel(const int chNum, const string &profileURI)
	{
		cout << "set up a new channel!!!" << endl;
		channel_type myChannel;
		myChannel.set_number(chNum);
		profile_pointer pp(this->uri2profile(profileURI));
		myChannel.set_profile(pp);
		this->track_channel(myChannel);

		message msg;
		if (pp->initialize(msg)) {
			connection_.async_send(this->channels_[chNum], msg,
								   bind(&basic_session::sent_channel_init,
										this,
										asio::placeholders::error,
										asio::placeholders::bytes_transferred,
										chNum));
		} else if (delegate_) {
			delegate_->channel_is_ready(this->channels_[chNum]);
		}
	}

	void
	sent_channel_init(const boost::system::error_code &error,
					  size_t bytes_transferred, const int chNum)
	{
		cout << "session::sent_channel_init" << endl;
		if (delegate_) {
			delegate_->channel_is_ready(this->channels_[chNum]);
		}
	}

	void
	on_sent_channel_start(const boost::system::error_code &error,
						  size_t bytes_transferred,
						  const int channel_number)
	{
		if (!error) {
			cout << "new channel " << channel_number << " is up!" << endl;
			cb_container::iterator i = chcb_.find(channel_number);
			if (i != chcb_.end()) {
				cout << "Invoke channel ready callback now!" << endl;
				i->second();
				chcb_.erase(i);
			} else {
				cout << "Could not find the channel ready callback." << endl;
			}
		}
	}

	void
	setup_tuning_channel()
	{
		channel_type tuning;
		tuning.set_profile(tuneprof_);

		// tuning channel number is always zero
		channels_.insert(channels_.begin(), tuning);
	}

	channel_type &tuning_channel() { return channels_[0]; } // by definition

	void
	standup_channel(const channel_type &channel)
	{
		ostringstream encstrm;
		if (channel.get_profile() && tuneprof_->add_channel(channel, encstrm)) {
			const string myBuffer(encstrm.str());
			message msg;
			tuneprof_->init_add_message(myBuffer, msg);

			connection_.async_send(tuning_channel(), msg,
								   bind(&basic_session::on_sent_channel_start,
										this,
										asio::placeholders::error,
										asio::placeholders::bytes_transferred,
										channel.number()));
		}
	}

	void
	track_channel(const channel_type &channel)
	{
		if (channels_.size() <= channel.number()) {
			channels_.resize(channel.number() * 2);
		}
		channels_[channel.number()] = channel;
		nextchan_ += 2;
	}

	profile_pointer
	uri2profile(const string &uri) const
	{
		/// \todo search for the URI in the profiles container
		profile_pointer pp(profiles_.front());
		return pp;
	}
private:
	error_callback            error_handler_;
public:
	void set_error_handler(error_callback cb) { error_handler_ = cb; }
};     // class basic_session

}      // namespace beep
#endif // BEEP_SESSION_HEAD
