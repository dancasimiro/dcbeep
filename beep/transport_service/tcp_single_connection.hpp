/// \file  tcp_single_connection.hpp
/// \brief Map the BEEP session onto a single the TCP/IP connection
///
/// UNCLASSIFIED
#ifndef BEEP_TCP_SINGLE_CONNECTION_HEAD
#define BEEP_TCP_SINGLE_CONNECTION_HEAD 1

namespace beep {

/// \brief TCP/IP BEEP transport layer using ASIO
/// \note this concept can be extended to support TLS and possible SASL
template <class StreamType = asio::ip::tcp::socket,
		  class AcceptorType = asio::ip::tcp::acceptor>
class basic_tcptl : private noncopyable {
public:
	typedef StreamType                            stream_type;
	typedef AcceptorType                          acceptor_type;
	typedef typename acceptor_type::endpoint_type endpoint_type;
	typedef basic_connection<stream_type>         connection_type;
	typedef asio::ip::tcp::resolver               resolver_type;

	basic_tcptl(io_service &service)
		: service_(service)
	{
	}

	io_service &lowest_layer() { return service_; }

private:
	io_service&               service_;
};

typedef basic_tcptl<> tcptl;

}      // namespace beep
#endif // BEEP_TCP_SINGLE_CONNECTION_HEAD
