/// \file  session.hpp
/// \brief BEEP session built on Boost ASIO
///
/// UNCLASSIFIED
#ifndef BEEP_SESSION_HEAD
#define BEEP_SESSION_HEAD 1
namespace beep {

/// \brief BEEP session management
///
/// \note Each session has an implicit tuning channel that performs
///       initialization.
template <class TransportLayer>
class basic_session {
public:
	typedef TransportLayer                        transport_layer;
	typedef transport_layer&                      transport_layer_reference;
	typedef basic_session<transport_layer>        base_type;
	typedef typename transport_layer::connection_type  connection_type;
	typedef connection_type&                      connection_reference;
	typedef basic_channel<base_type>              channel_type;
	typedef shared_ptr<profile>                   profile_pointer;

	enum role {
		initiating_role,
		listening_role,
	};

	basic_session(transport_layer_reference transport)
		: transport_(transport)
		, connection_(transport_.lowest_layer())
		, channels_()
		, profiles_()
		, role_(initiating_role)
		, nextchan_(1)
	{
		connection_.install_message_callback(bind(&basic_session::handle_message,
												  this, _1));
		setup_tuning_channel();
	}

	basic_session(transport_layer_reference transport, role r)
		: transport_(transport)
		, connection_(transport_.lowest_layer())
		, channels_()
		, profiles_()
		, role_(r)
		, nextchan_(r == initiating_role ? 1 : 2)
	{
		connection_.install_message_callback(bind(&basic_session::handle_message,
												  this, _1));
		setup_tuning_channel();
	}

	connection_reference connection() { return connection_; }

	void start()
	{
		connection_.start();
		connection_.transmit();
	}

	unsigned int next_channel_number() const { return nextchan_; }

	void add(channel_type &channel)
	{
		static const size_t max_tries = 4;
		start_message myMessage;
		myMessage.request_num = channel.number();
		myMessage.profile_uri = channel.get_profile()->get_uri();
		vector<char> encMem;
		size_t encLen = 0;
		size_t attempt = 0;

		// get management syntax
		profile_pointer pp(tuning_channel().get_profile());

		encMem.resize(256, '\0');
		encLen = encMem.size();
		while (!pp->encode(start_message::code(), &myMessage,
						   sizeof(start_message), &encMem[0], &encLen) &&
			   attempt++ < max_tries) {
			encMem.resize(4 * encMem.size());
			encLen = encMem.size();
		}

		if (attempt < max_tries) {
			message msg;
			pp->setup_message(start_message::code(), &encMem[0], encLen, msg);

			tuning_channel().encode(msg, connection_);
			connection_.transmit();

			if (channels_.size() <= channel.number()) {
				channels_.resize(channel.number() * 2);
			}
			channels_[channel.number()] = channel;
			nextchan_ += 2;
#if 1
			message newChannelInit;
			if (channel.get_profile()->initialize(newChannelInit)) {
				channel.encode(newChannelInit, connection_);
			}
#endif
		}
	}

	void install(profile_pointer pp)
	{
		profiles_.push_back(pp);
	}
private:
	typedef vector<channel_type>                  channels_container;
	typedef list<profile_pointer>                 profiles_container;

	transport_layer_reference transport_;
	connection_type           connection_;
	channels_container        channels_;
	profiles_container        profiles_;
	role                      role_;
	unsigned int              nextchan_;

	void
	handle_message(const message &msg)
	{
		// set up a try/catch block... (???)
		cout << "Handle an incoming message for channel #" << msg.channel_number() << endl;
		// special case for tuning channel...
		// this hard codes that the incoming message was a request to add
		// a channel (1) with the only supported profile...
#if 1
		if (msg.channel_number() == 0 && msg.type() == frame::msg) {
			cout << "set up a new channel in the listener..." << endl;
			channel_type new_channel;
			new_channel.set_number(1);
			new_channel.set_profile(profiles_.back());
			if (channels_.size() <= new_channel.number()) {
				channels_.resize(new_channel.number() * 2);
			}
			channels_[new_channel.number()] = new_channel;
		}
#endif

		message theReply;
		channel_type &theChannel(channels_[msg.channel_number()]);
		if (theChannel.get_profile()->handle(msg, theReply)) {
			theChannel.encode(theReply, connection_);
			connection_.transmit();
		}
	}
	
	void
	setup_tuning_channel()
	{
		profile_pointer tpp(new channel_management_profile);
		profiles_.push_back(tpp);

		channel_type tuning;
		tuning.set_profile(tpp);

		message msg;
		if (tpp->initialize(msg)) {
			tuning.encode(msg, connection_);
		}
		// tuning channel number is always zero
		channels_.insert(channels_.begin(), tuning);
	}

	channel_type &tuning_channel() { return channels_[0]; } // by definition
};     // class basic_session

}      // namespace beep
#endif // BEEP_SESSION_HEAD
