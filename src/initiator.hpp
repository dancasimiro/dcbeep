/// \file  initiator.hpp
/// \brief BEEP session in the initiating role
///
/// UNCLASSIFIED
#ifndef BEEP_INITIATING_HEAD
#define BEEP_INITIATING_HEAD 1
namespace beep {

template <class SessionType>
class basic_initiator {
public:
	typedef SessionType                                     session_type;
	typedef session_type&                                   session_reference;
	typedef typename session_type::transport_layer          transport_layer;
	typedef typename transport_layer::connection_type       connection_type;
	typedef typename transport_layer::endpoint_type         endpoint_type;

	basic_initiator(transport_layer &transport)
		: session_(transport, initiating_role)
	{
		session_.set_error_handler(::bind(&basic_initiator::on_session_error, this, _1));
	}

	session_reference next_layer() { return session_; }
	io_service &lowest_layer() { return session_.lowest_layer(); }

	void connect(const endpoint_type &endpoint)
	{
		session_.connection().lowest_layer().connect(endpoint);
		session_.start();
	}

	template <class Handler>
	void async_connect(const endpoint_type &endpoint, Handler handler)
	{
		handle_ = handler;
		session_.connection().lowest_layer()
			.async_connect(endpoint,
						   bind(&basic_initiator::on_connect,
								this,
								placeholders::error));
	}
private:
	typedef function<void (boost::system::error_code)> handler_type;

	session_type              session_;
	handler_type              handle_;

	void on_connect(const boost::system::error_code &error)
	{
		if (!error) {
			session_.start();
		}
		if (!handle_.empty()) {
			handle_(error);
		}
	}

	void on_session_error(const boost::system::error_code &error)
	{
	}
};     // class basic_initiator

}      // namespace beep
#endif // BEEP_INITIATING_HEAD
