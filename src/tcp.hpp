/// \file  tcp.hpp
/// \brief Map the TCP/IP transport service to BEEP
///
/// UNCLASSIFIED
#ifndef BEEP_TCP_HEAD
#define BEEP_TCP_HEAD 1
namespace beep {

/// \brief TCP/IP BEEP transport layer using ASIO
/// \note this concept can be extended to support TLS and possible SASL
class tcptl : private noncopyable {
public:
	typedef ip::tcp::socket                       stream_type;
	typedef ip::tcp::acceptor                     acceptor_type;
	typedef stream_type::endpoint_type            endpoint_type;
	typedef basic_session<tcptl>                  session_type;
	typedef basic_connection<stream_type>         connection_type;

	tcptl(io_service &service)
		: service_(service)
	{
	}

	io_service &lowest_layer() { return service_; }

private:
	io_service&               service_;
};

}      // namespace beep
#endif // BEEP_TCP_MAPPING_HEAD
