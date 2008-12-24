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

	basic_listener(transport_layer &transport)
		: transport_(transport)
		, acceptor_(transport_.lowest_layer())
		, next_(new session_type(transport_, listening_role))
		, sessions_()
		, profiles_()
	{
	}

	basic_listener(transport_layer &transport, const endpoint_type &endpoint)
		: transport_(transport)
		, acceptor_(transport_.lowest_layer(), endpoint)
		, next_(new session_type(transport_, listening_role))
		, sessions_()
		, profiles_()
	{
		this->start();
	}

	template <class Profile, class Handler>
	void install(Profile prof, Handler handler)
	{
		profiles_.insert(make_pair(prof, handler));
		next_->install(prof, handler);
		typedef typename sessions_container::iterator iterator;
		for (iterator i = sessions_.begin(); i != sessions_.end(); ++i) {
			(*i)->install(prof, handler);
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
		acceptor_.async_accept(next_->connection_layer(),
							   ::bind(&basic_listener::on_accept, this,
									  asio::placeholders::error));
	}

	void stop()
	{
		typedef typename sessions_container::iterator iterator;
		acceptor_.close();
		for (iterator i = sessions_.begin(); i != sessions_.end(); ++i) {
			(*i)->stop(::bind(&basic_listener::handle_stop,
							  this,
							  _1, _2, _3));
		}
	}
protected:
	void remove(session_type &aSession)
	{
		typedef typename sessions_container::iterator iterator;
		for (iterator i = sessions_.begin(); i != sessions_.end(); ++i) {
			if (i->get() == &aSession) {
				sessions_.erase(i);
				break;
			}
		}
	}
private:
	typedef list<session_pointer>                           sessions_container;
	typedef typename session_type::profile_container        profile_container;

	transport_layer&          transport_;
	acceptor_type             acceptor_;
	session_pointer           next_; // next session
	sessions_container        sessions_;
	profile_container         profiles_;

	void on_accept(const boost::system::error_code &error)
	{
		typedef typename profile_container::const_iterator const_iterator;
		if (!error) {
			sessions_.push_back(next_);
			next_->start();
			session_pointer
				nextSession(new session_type(transport_, listening_role));
			next_ = nextSession;
			next_->install(profiles_.begin(), profiles_.end());
			this->start();
		} else {
			cerr << "Problem: " << error.message() << endl;
		}
	}

	void handle_stop(const beep::reply_code status,
					 const session_type &theSession, const beep::channel &theChannel)
	{
		if (status == beep::success) {
			typedef typename sessions_container::iterator iterator;
			for (iterator i = sessions_.begin(); i != sessions_.end(); ++i) {
				if (i->get() == &theSession) {
					sessions_.erase(i);
					break;
				}
			}
		}
	}
};

}      // namespace beep
#endif // BEEP_LISTENER_HEAD
