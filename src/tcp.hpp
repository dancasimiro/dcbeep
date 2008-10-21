/// \file  tcp.hpp
/// \brief Map the TCP/IP transport service to BEEP
///
/// UNCLASSIFIED
#ifndef BEEP_TCP_HEAD
#define BEEP_TCP_HEAD 1
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
#endif // BEEP_TCP_MAPPING_HEAD
