/// \file  initiator.hpp
/// \brief BEEP session in the initiating role
///
/// UNCLASSIFIED
#ifndef BEEP_INITIATING_HEAD
#define BEEP_INITIATING_HEAD 1
namespace beep {

template <class SessionType>
class basic_initiator : private noncopyable {
public:
	typedef SessionType                                     session_type;
	typedef session_type&                                   session_reference;
	typedef typename session_type::transport_layer          transport_layer;
	typedef typename transport_layer::connection_type       connection_type;
	typedef typename transport_layer::endpoint_type         endpoint_type;

	basic_initiator(transport_layer &transport)
		: session_(transport, initiating_role)
	{
	}

	virtual ~basic_initiator()
	{
	}

	session_reference next_layer() { return session_; }
	io_service &lowest_layer() { return session_.lowest_layer(); }

	template <class Handler>
	void async_connect(const endpoint_type &endpoint, Handler handler)
	{
		session_.connection_.lowest_layer()
			.async_connect(endpoint,
						   on_connect_handler_helper<Handler>(*this, handler));
	}
private:
	session_type              session_;

	template <class Handler>
	struct on_connect_handler_helper {
		basic_initiator       &theInitiator;
		Handler               theHandler;

		on_connect_handler_helper(basic_initiator &init, Handler h)
			: theInitiator(init)
			, theHandler(h)
		{ }

		void operator()(const boost::system::error_code &error)
		{
			if (!error) {
				theInitiator.session_.start();
			}
			theHandler(error, theInitiator.session_);
		}
	};
};     // class basic_initiator

}      // namespace beep
#endif // BEEP_INITIATING_HEAD
