/// \file  listener.hpp
/// \brief BEEP session in the listening role
///
/// UNCLASSIFIED
#ifndef BEEP_LISTENER_HEAD
#define BEEP_LISTENER_HEAD 1
namespace beep {

/// Create new sessions
template <class SessionType>
class basic_listener {
public:
	typedef SessionType                                     session_type;
	typedef session_type&                                   session_reference;
	typedef shared_ptr<session_type>                        session_pointer;
	typedef typename session_type::transport_layer          transport_layer;
	typedef typename transport_layer::acceptor_type         acceptor_type;
	typedef typename transport_layer::endpoint_type         endpoint_type;
	typedef typename acceptor_type::protocol_type           protocol_type;
	typedef typename acceptor_type::native_type             native_acceptor;   
	typedef shared_ptr<profile>                             profile_pointer;

	struct delegate {
		virtual void session_is_ready(basic_listener &listener,
									  session_reference session) = 0;
	};

	basic_listener(transport_layer &transport)
		: transport_(transport)
		, acceptor_(transport_.lowest_layer())
		, next_(new session_type(transport_, listening_role))
		, sessions_()
		, profiles_()
		, delegate_(NULL)
	{
		next_->set_error_handler(::bind(&basic_listener::on_session_error, this, _1));
	}

	basic_listener(transport_layer &transport, const endpoint_type &endpoint)
		: transport_(transport)
		, acceptor_(transport_.lowest_layer(), endpoint)
		, next_(new session_type(transport_, listening_role))
		, sessions_()
		, profiles_()
		, delegate_(NULL)
	{
		next_->set_error_handler(::bind(&basic_listener::on_session_error, this, _1));

		this->start();
	}

	void install(profile_pointer pp)
	{
		profiles_.push_back(pp);
		next_->install(pp);
		typedef typename sessions_container::iterator iterator;
		for (iterator i = sessions_.begin(); i != sessions_.end(); ++i) {
			(*i)->install(pp);
		}
	}

	void close()
	{
		acceptor_.close();
	}

	void assign(const protocol_type &protocol, const native_acceptor &native)
				
	{
		acceptor_.assign(protocol, native);
	}

	void open(const protocol_type &protocol)
	{
		acceptor_.open(protocol);
	}

	template <typename SettableSocketOption>
	void set_option(const SettableSocketOption &option)
	{
		acceptor_.set_option(option);
	}

	void bind(const endpoint_type &endpoint)
	{
		acceptor_.bind(endpoint);
	}

	void listen(int backlog = asio::socket_base::max_connections)
	{
		acceptor_.listen(backlog);
	}

	void start()
	{
		acceptor_.async_accept(next_->connection().lowest_layer(),
							   ::bind(&basic_listener::on_accept, this,
									  asio::placeholders::error));
	}

	void stop()
	{
		acceptor_.close();
		for_each(sessions_.begin(), sessions_.end(),
				 mem_fn(&session_type::close));
		sessions_.clear();
	}

	void set_delegate(delegate *aDelegate) { delegate_ = aDelegate; }
private:
	typedef list<session_pointer>                           sessions_container;
	typedef list<profile_pointer>                           profiles_container;

	transport_layer&          transport_;
	acceptor_type             acceptor_;
	session_pointer           next_; // next session
	sessions_container        sessions_;
	profiles_container        profiles_;
	delegate                  *delegate_;

	void on_accept(const boost::system::error_code &error)
	{
		typedef profiles_container::const_iterator const_iterator;
		if (!error) {
			cout << "Accepted a connection!" << endl;
			sessions_.push_back(next_);
			if (delegate_) {
				delegate_->session_is_ready(*this, *next_);
			}
			next_->start();
			session_pointer
				nextSession(new session_type(transport_, listening_role));
			nextSession->set_error_handler(::bind(&basic_listener::on_session_error, this, _1));
			next_ = nextSession;
			for (const_iterator i=profiles_.begin(); i!=profiles_.end();++i) {
				next_->install(*i);
			}
			this->start();
		} else {
			cerr << "Problem: " << error.message() << endl;
		}
	}

	void on_session_error(const boost::system::error_code &error)
	{
		//cout << "closing all of the sessions and the acceptor." << endl;
		// this will close __all__ of the sessions if one disconnects
		// this is OK short term because there is only one remote site.
		//this->stop();
	}
};

}      // namespace beep
#endif // BEEP_LISTENER_HEAD
