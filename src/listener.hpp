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
	typedef shared_ptr<profile>                             profile_pointer;

	basic_listener(transport_layer &transport, const endpoint_type &endpoint)
		: transport_(transport)
		, acceptor_(transport_.lowest_layer())
		, next_(new session_type(transport_, session_type::listening_role))
		, sessions_()
	{
		acceptor_.open(endpoint.protocol());
		acceptor_.set_option(socket_base::reuse_address(true));
		acceptor_.bind(endpoint);
		acceptor_.listen();

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
private:
	typedef list<session_pointer>                           sessions_container;
	typedef list<profile_pointer>                           profiles_container;

	transport_layer&          transport_;
	acceptor_type             acceptor_;
	session_pointer           next_; // next session
	sessions_container        sessions_;
	profiles_container        profiles_;

	void start()
	{
		acceptor_.async_accept(next_->connection().lowest_layer(),
							   bind(&basic_listener::on_accept, this,
									placeholders::error));
	}

	void on_accept(const boost::system::error_code &error)
	{
		typedef profiles_container::const_iterator const_iterator;
		if (!error) {
			cout << "Accepted a connection!" << endl;
			sessions_.push_back(next_);
			next_->start();
			session_pointer
				nextSession(new session_type(transport_,
											 session_type::listening_role));
			next_ = nextSession;
			for (const_iterator i=profiles_.begin(); i!=profiles_.end();++i) {
				next_->install(*i);
			}
			this->start();
		} else {
			cerr << "Problem: " << error.message() << endl;
		}
	}
};

}      // namespace beep
#endif // BEEP_LISTENER_HEAD
