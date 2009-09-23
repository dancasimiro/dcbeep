/// \file  solo-tcp.hpp
/// \brief Map the BEEP session onto a single the TCP/IP connection
///
/// UNCLASSIFIED
#ifndef BEEP_SOLO_TCP_HEAD
#define BEEP_SOLO_TCP_HEAD 1

#include <boost/asio.hpp>
#include "basic-solo-stream.hpp"

namespace beep {
namespace transport_service {

typedef basic_solo_stream_initiator<boost::asio::ip::tcp::socket> solo_tcp_initiator;

}      // namespace transport_service
}      // namespace beep
#endif // BEEP_SOLO_TCP_HEAD
