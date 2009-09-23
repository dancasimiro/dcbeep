/// \file  basic-solo-stream.hpp
/// \brief Map the BEEP session onto a single stream connection
///
/// UNCLASSIFIED
#ifndef BEEP_SOLO_STREAM_HEAD
#define BEEP_SOLO_STREAM_HEAD 1

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/signals2.hpp>

#include "beep/frame.hpp"

namespace beep {
namespace transport_service {
namespace detail {

/// \brief Reliably transmit beep frames to and from the network
///
/// This class reads frames from the network and combines them to form complete beep messages.
/// It also takes beep messages and encodes them into frames that are transmitted.
///
/// Finally, it implements the flow control defined in RFC 3081.
template <class StreamT>
class solo_stream_service_impl : public boost::enable_shared_from_this<solo_stream_service_impl<StreamT> > {
public:
	typedef StreamT stream_type;

	solo_stream_service_impl(boost::asio::io_service &service)
		: stream_(service)
		, rsb_()
	{
	}

	// Take the BEEP frames and send them over the network
	template <typename FwdIterator>
	void send_frames(FwdIterator first, FwdIterator last)
	{
	}
private:
	typedef boost::asio::streambuf streambuf_type;

	stream_type    stream_;
	streambuf_type rsb_; // read streambuf
};
	
}

/// \brief BEEP single stream connection transport layer using ASIO
/// \note this concept can be extended to support TLS and possible SASL
template <class StreamType>
class basic_solo_stream : private boost::noncopyable {
public:
protected:
	typedef StreamType   stream_type;
	typedef stream_type& stream_reference;

	typedef boost::signals2::signal<void (const frame&)> new_frame_signal;
	typedef boost::signals2::connection                  signal_connection;

	basic_solo_stream()
		: pimpl_()
	{
	}

public:
	/// \brief Get notified when new frames arrive
	///
	/// Use this member function to install a callback function that is
	/// invoked whenever a new frame arrives from the network.
	signal_connection subscribe(new_frame_signal::slot_function_type slot)
	{
		signal_connection placeholder;
		return placeholder;
	}

	/// \brief Send frames to the remote endpoint
	///
	/// Put a sequence of frames onto the network
	template <typename FwdIterator>
	void send_frames(const FwdIterator first, const FwdIterator last)
	{
	}
private:
	typedef detail::solo_stream_service_impl<stream_type> impl_type;
	typedef boost::shared_ptr<impl_type>                  pimpl_type;

	pimpl_type pimpl_;
};     // class basic_solo_stream
}      // namespace transport_service
}      // namespace beep
#endif // BEEP_SOLO_STREAM_HEAD
