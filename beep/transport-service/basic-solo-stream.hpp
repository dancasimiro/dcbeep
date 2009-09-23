/// \file  basic-solo-stream.hpp
/// \brief Map the BEEP session onto a single stream connection
///
/// UNCLASSIFIED
#ifndef BEEP_SOLO_STREAM_HEAD
#define BEEP_SOLO_STREAM_HEAD 1

#include <cassert>
#include <istream>
#include <list>
#include <algorithm>

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/signals2.hpp>

#include "beep/frame.hpp"
#include "beep/frame-stream.hpp"

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
	typedef StreamT                 stream_type;
	typedef stream_type&            stream_reference;
	typedef boost::asio::io_service service_type;
	typedef service_type&           service_reference;

	solo_stream_service_impl(service_reference service)
		: stream_(service)
		, rsb_()
		, wsb_()
		, fwsb_(&wsb_[0])
		, bwsb_(&wsb_[1])
		, wstrand_(service)
	{
	}

	stream_reference get_stream() { return stream_; }

	void send_frame(const frame &aFrame)
	{
		using boost::bind;
		wstrand_.dispatch(bind(&solo_stream_service_impl::serialize_frame,
							   this->shared_from_this(),
							   aFrame));
		wstrand_.dispatch(bind(&solo_stream_service_impl::do_send_if_possible,
							   this->shared_from_this()));
	}

	// Take the BEEP frames and send them over the network
	template <typename FwdIterator>
	void send_frames(FwdIterator first, const FwdIterator last)
	{
		using boost::bind;
		using std::for_each;
		for (; first != last; ++first) {
			wstrand_.dispatch(bind(&solo_stream_service_impl::serialize_frame,
								   this->shared_from_this(), *first));
		}
		wstrand_.dispatch(bind(&solo_stream_service_impl::do_send_if_possible,
							   this->shared_from_this()));
	}

	void start()
	{
	}

	void set_error(const boost::system::error_code &/*error*/)
	{
	}
private:
	typedef boost::asio::streambuf streambuf_type;
	typedef service_type::strand   strand_type;

	stream_type    stream_;
	streambuf_type rsb_; // read streambuf
	streambuf_type wsb_[2]; // double buffered write streambufs
	streambuf_type *fwsb_;  // current (foreground) sendbuf that is being sent
	streambuf_type *bwsb_;  // background streambuf that is updated while fwsb_ is transmitted
	strand_type    wstrand_; // serialize write operations

	void serialize_frame(const frame &frame)
	{
		using std::ostream;
		ostream stream(bwsb_);
		if (!(stream << frame)) {
			/// \todo send an error up the chain
			assert(false);
		}
	}

	void do_send_if_possible()
	{
		if (fwsb_->size() == 0 && stream_.is_open()) {
			do_send();
		}
	}

	void do_send()
	{
		using boost::swap;
		using boost::bind;
		using namespace boost::asio;

		swap(bwsb_, fwsb_);
		async_write(stream_, *fwsb_,
					wstrand_.wrap(bind(&solo_stream_service_impl::handle_send,
									   this->shared_from_this(),
									   placeholders::error,
									   placeholders::bytes_transferred)));
	}

	void handle_send(const boost::system::error_code &error,
					 std::size_t /*bytes_transferred*/)
	{
		using boost::bind;
		if (!error || error == boost::asio::error::message_size) {
			if (bwsb_->size()) {
				wstrand_.dispatch(bind(&solo_stream_service_impl::do_send,
									   this->shared_from_this()));
			}
		} else {
			stream_.close();
			fwsb_->consume(fwsb_->size());
			bwsb_->consume(bwsb_->size());
		}
	}
};

template <class T>
void handle_initiator_connection(const boost::system::error_code &error,
								 const T service_impl)
{
	if (!error) {
		service_impl->start();
	} else {
		service_impl->set_error(error);
	}
}
	
}      // namespace detail

/// \brief BEEP single stream connection transport layer using ASIO
/// \note this concept can be extended to support TLS and possible SASL
template <class StreamType>
class basic_solo_stream : private boost::noncopyable {
public:
	typedef StreamType                  stream_type;
	typedef stream_type&                stream_reference;
	typedef boost::system::error_code   error_code_type;
	typedef boost::signals2::connection signal_connection;

	typedef boost::signals2::signal<void (const error_code_type&, const frame&)> new_frame_signal;

	basic_solo_stream()
		: connections_()
	{
	}

	virtual ~basic_solo_stream() {}

	/// \brief Get notified when new frames arrive
	///
	/// Use this member function to install a callback function that is
	/// invoked whenever a new frame arrives from the network.
	signal_connection subscribe(new_frame_signal::slot_function_type slot)
	{
		signal_connection c;
		return c;
	}

	void send_frame(const frame &aFrame)
	{
		using std::for_each;
		using boost::bind;
		for_each(connections_.begin(), connections_.end(),
				 bind(&impl_type::send_frame, _1, aFrame));
	}

	/// \brief Send frames to the remote endpoint
	///
	/// Put a sequence of frames onto the network
	template <typename FwdIterator>
	void send_frames(const FwdIterator first, const FwdIterator last)
	{
		typedef typename container_type::iterator iterator;
		for (iterator i = connections_.begin(); i != connections_.end(); ++i) {
			(*i)->send_frames(first, last);
		}
	}

	//bool create_channel(const channel &chan)
protected:
	typedef detail::solo_stream_service_impl<stream_type> impl_type;
	typedef boost::shared_ptr<impl_type>                  pimpl_type;

	void add_connection(const pimpl_type connection)
	{
		connections_.push_back(connection);
	}

	void remove_connection(const pimpl_type connection)
	{
		connections_.remove(connection);
	}
private:
	typedef std::list<pimpl_type> container_type;
	container_type connections_;
};     // class basic_solo_stream

/// \brief Actively open a TCP connection
///
/// Start a new connection by setting the remote endpoint
template <class StreamT>
class basic_solo_stream_initiator : public basic_solo_stream<StreamT> {
public:
	typedef StreamT                             stream_type;
	typedef stream_type&                        stream_reference;
	typedef boost::asio::io_service             service_type;
	typedef service_type&                       service_reference;
	typedef typename stream_type::endpoint_type endpoint_type;
	
	basic_solo_stream_initiator(service_reference service)
		: basic_solo_stream<StreamT>()
		, service_(service)
		, current_()
		, ep_()
	{
	}

	virtual ~basic_solo_stream_initiator() {}

	void set_endpoint(const endpoint_type &ep)
	{
		using boost::bind;
		using boost::swap;
		using namespace boost::asio;

		ep_ = ep;
		pimpl_type next(new impl_type(service_));
		if (current_) {
			this->remove_connection(current_);
		}
		swap(current_, next);
		this->add_connection(current_);
		current_->get_stream().async_connect(ep_,
											 bind(detail::handle_initiator_connection<pimpl_type>,
												  placeholders::error,
												  current_));
	}
private:
	typedef basic_solo_stream<stream_type>  super_type;
	typedef typename super_type::impl_type  impl_type;
	typedef typename super_type::pimpl_type pimpl_type;
	const service_reference service_;
	pimpl_type    current_; // current connection
	endpoint_type ep_;
};     // class basic_solo_stream_initiator

/// \brief Passively wait for a new TCP connection.
///
/// Listen for incoming connections
template <class StreamT>
class basic_solo_stream_listener : public basic_solo_stream<StreamT> {
public:
	typedef StreamT                             stream_type;
	typedef stream_type&                        stream_reference;
	typedef typename stream_type::service_type  service_type;
	typedef service_type&                       service_reference;
	typedef typename stream_type::protocol_type protocol_type;
	typedef typename protocol_type::acceptor    acceptor_type;

	basic_solo_stream_listener(service_reference service)
		: basic_solo_stream<stream_type>()
	{
	}
private:
	//acceptor_type           acceptor_;
};     // class basic_solo_stream_listener
}      // namespace transport_service
}      // namespace beep
#endif // BEEP_SOLO_STREAM_HEAD
