/// \file  solo-tcp.hpp
/// \brief Map the BEEP session onto a single the TCP/IP connection
///
/// UNCLASSIFIED
#ifndef BEEP_SOLO_TCP_HEAD
#define BEEP_SOLO_TCP_HEAD 1

#include <boost/noncopyable.hpp>
#include <boost/asio.hpp>

namespace beep {
namespace transport_service {

/// \brief TCP/IP BEEP transport layer using ASIO
/// \note this concept can be extended to support TLS and possible SASL
template <class StreamType = boost::asio::ip::tcp::socket,
		  class AcceptorType = boost::asio::ip::tcp::acceptor>
class basic_solo_tcp : private boost::noncopyable {
public:
	typedef boost::asio::io_service               service_type;
	typedef service_type&                         service_reference;
	typedef StreamType                            stream_type;
	typedef AcceptorType                          acceptor_type;
	typedef typename acceptor_type::endpoint_type endpoint_type;
	typedef boost::asio::ip::tcp::resolver        resolver_type;

	basic_solo_tcp(service_reference service)
		: service_(service)
	{
	}

	service_reference lowest_layer() { return service_; }

private:
	const service_reference service_;
};

typedef basic_solo_tcp<> solo_tcp;

}      // namespace transport_service
}      // namespace beep
#endif // BEEP_SOLO_TCP_HEAD
