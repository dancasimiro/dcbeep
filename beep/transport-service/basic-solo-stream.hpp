/// \file  basic-solo-stream.hpp
/// \brief Map the BEEP session onto a single stream connection
///
/// UNCLASSIFIED
#ifndef BEEP_SOLO_STREAM_HEAD
#define BEEP_SOLO_STREAM_HEAD 1

#include <cassert>
#include <utility>
#include <istream>
#include <sstream>
#include <map>

#include <boost/noncopyable.hpp>
#include <boost/throw_exception.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/signals2.hpp>

#include "beep/identifier.hpp"
#include "beep/role.hpp"
#include "beep/frame.hpp"
#include "beep/frame-stream.hpp"

namespace beep {

struct asio_frame_parser
{
	typedef frame_parser<boost::asio::buffers_iterator<boost::asio::streambuf::const_buffers_type> > parser_type;

	template <typename Iterator>
	std::pair<Iterator, bool> operator()(Iterator begin, Iterator end) const
	{
		using std::make_pair;
		using qi::parse;
		static parser_type g;
		const bool success = parse(begin, end, g);
		return make_pair(begin, success);
	}
};
}      // namespace beep
	

namespace boost {
namespace asio {
template <>
struct is_match_condition<beep::asio_frame_parser>
	: public boost::true_type {};
}      // namespace asio
}      // namespace boost

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

	typedef boost::signals2::connection signal_connection;
	typedef boost::system::error_code   error_code_type;
	typedef boost::signals2::signal<void (const error_code_type&, const frame&)> frame_signal_t;

	typedef boost::function<void (const error_code_type&, const identifier&)> network_cb_t;

	solo_stream_service_impl(service_reference service)
		: stream_(service)
		, rsb_()
		, wsb_()
		, fwsb_(&wsb_[0])
		, bwsb_(&wsb_[1])
		, wstrand_(service)
		, signal_frame_()
		, net_changed_()
		, matcher_()
	{
	}

	stream_reference get_stream() { return stream_; }
	void set_network_callback(const network_cb_t &cb) { net_changed_ = cb; }

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
		for (; first != last; ++first) {
			wstrand_.dispatch(bind(&solo_stream_service_impl::serialize_frame,
								   this->shared_from_this(), *first));
		}
		wstrand_.dispatch(bind(&solo_stream_service_impl::do_send_if_possible,
							   this->shared_from_this()));
	}

	signal_connection subscribe(const frame_signal_t::slot_type slot)
	{
		return signal_frame_.connect(slot);
	}

	void start(const boost::system::error_code &error, const identifier &id)
	{
		/// Reset all of the streambuf objects
		fwsb_->consume(fwsb_->size());
		bwsb_->consume(bwsb_->size());
		rsb_.consume(rsb_.size());
		assert(fwsb_->size() == 0);
		assert(bwsb_->size() == 0);
		assert(rsb_.size() == 0);
		net_changed_(error, id);
		if (!error) {
			do_start_read();
		}
	}
private:
	typedef boost::asio::streambuf streambuf_type;
	typedef service_type::strand   strand_type;
	typedef asio_frame_parser      matcher_type;

	stream_type    stream_;
	streambuf_type rsb_; // read streambuf
	streambuf_type wsb_[2]; // double buffered write streambufs
	streambuf_type *fwsb_;  // current (foreground) sendbuf that is being sent
	streambuf_type *bwsb_;  // background streambuf that is updated while fwsb_ is transmitted
	strand_type    wstrand_; // serialize write operations
	frame_signal_t signal_frame_;
	network_cb_t   net_changed_;
	matcher_type   matcher_;

	void set_error(const boost::system::error_code &error)
	{
		boost::system::error_code close_error;
		stream_.close(close_error);
		fwsb_->consume(fwsb_->size());
		bwsb_->consume(bwsb_->size());
		signal_frame_(error, beep::frame());
		if (close_error) {
			signal_frame_(close_error, beep::frame());
		}
	}

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
		if (fwsb_->size() == 0 && stream_.lowest_layer().is_open() && bwsb_->size() > 0) {
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

	void
	do_start_read()
	{
		using boost::bind;
		using namespace boost::asio;
		async_read_until(stream_, rsb_,
						 matcher_,
						 bind(&solo_stream_service_impl::handle_frame_read,
							  this->shared_from_this(),
							  placeholders::error,
							  placeholders::bytes_transferred));
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
			set_error(error);
		}
	}

	void handle_frame_read(const boost::system::error_code &error,
						   std::size_t /*bytes_transferred*/)
	{
		using std::istream;
		if (!error || error == boost::asio::error::message_size) {
			istream stream(&rsb_);
			frame current;
			stream >> current;
			assert(stream);
			signal_frame_(boost::system::error_code(), current);
			do_start_read();
		} else {
			set_error(error);
		}
	}
};

template <class StreamT>
class acceptor_impl : public boost::enable_shared_from_this<acceptor_impl<StreamT> > {
public:
	typedef StreamT                               stream_type;
	typedef typename stream_type::protocol_type   protocol_type;
	typedef typename protocol_type::acceptor      acceptor_type;
	typedef boost::asio::io_service               service_type;
	typedef service_type&                         service_reference;
	typedef solo_stream_service_impl<stream_type> connection_type;
	typedef boost::shared_ptr<connection_type>    connection_pointer;
	typedef typename protocol_type::endpoint      endpoint_type;

	typedef boost::signals2::connection           signal_connection_t;
	typedef boost::signals2::signal<void (const boost::system::error_code&,
										  const connection_pointer)> accept_signal_t;

	acceptor_impl(const service_reference svc)
		: acceptor_(svc)
		, signal_()
	{
	}

	signal_connection_t subscribe(const typename accept_signal_t::slot_type slot)
	{
		return signal_.connect(slot);
	}

	void close()
	{
		acceptor_.close();
	}

	void accept_from(const endpoint_type &ep)
	{
		acceptor_.open(ep.protocol());
		acceptor_.set_option(typename stream_type::lowest_layer_type::reuse_address(true));
		acceptor_.bind(ep);
		acceptor_.listen(5);
	}

	void async_accept(const connection_pointer ptr)
	{
		using boost::bind;
		acceptor_.async_accept(ptr->get_stream().lowest_layer(),
							   bind(&acceptor_impl::handle_accept,
									this->shared_from_this(),
									boost::asio::placeholders::error,
									ptr));
	}
private:
	acceptor_type   acceptor_;
	accept_signal_t signal_;

	void handle_accept(const boost::system::error_code &error,
					   const connection_pointer ptr)
	{
		signal_(error, ptr);
	}
};

}      // namespace detail

class bad_session_error : public std::runtime_error {
public:
	bad_session_error(const char *msg) : std::runtime_error(msg) {}
	bad_session_error(const std::string &msg) : std::runtime_error(msg) {}
	virtual ~bad_session_error() throw () {}
};

/// \brief BEEP single stream connection transport layer using ASIO
/// \note this concept can be extended to support TLS and possible SASL
template <class StreamType, role RoleEnum>
class basic_solo_stream : private boost::noncopyable {
public:
	typedef StreamType                  stream_type;
	typedef stream_type&                stream_reference;
	typedef boost::signals2::connection signal_connection;

	typedef boost::signals2::signal<void (const boost::system::error_code&, const identifier&)> network_signal_t;
	typedef boost::signals2::signal<void (const boost::system::error_code&, const frame&)>      frame_signal_t;

	static role get_role() { return RoleEnum; }

	basic_solo_stream()
		: connections_()
		, network_signal_()
	{
	}

	virtual ~basic_solo_stream() {}

	void shutdown_connection(const beep::identifier &ident)
	{
		typedef typename container_type::iterator iterator;
		iterator i = connections_.find(ident);
		assert(i != connections_.end());
		if (i != connections_.end()) {
			i->second->get_stream().shutdown(stream_type::shutdown_send);
		}
	}

	// closes the associated socket and removes the connection object
	void stop_connection(const beep::identifier &ident)
	{
		typedef typename container_type::iterator iterator;
		iterator i = connections_.find(ident);
		assert(i != connections_.end());
		if (i != connections_.end()) {
			i->second->get_stream().close();
			connections_.erase(i);
		}
	}

	/// \brief Get notified when a new network session is established
	signal_connection install_network_handler(const network_signal_t::slot_type slot)
	{
		return network_signal_.connect(slot);
	}

	/// \brief Get notified when new frames arrive
	///
	/// Use this member function to install a callback function that is
	/// invoked whenever a new frame arrives from the network.
	signal_connection subscribe(const identifier &id,
								const frame_signal_t::slot_type slot)
	{
		const typename container_type::iterator i = connections_.find(id);
		assert(i != connections_.end());
		if (i == connections_.end()) {
			std::ostringstream strm;
			strm << "Session " << id << " is not recognized.";
			BOOST_THROW_EXCEPTION(bad_session_error(strm.str()));
		}
		return i->second->subscribe(slot);
	}

	void send_frame(const identifier &ident, const frame &aFrame)
	{
		typedef typename container_type::iterator iterator;
		iterator i = connections_.find(ident);
		assert(i != connections_.end());
		if (i != connections_.end()) {
			i->second->send_frame(aFrame);
		}
	}

	/// \brief Send frames to the remote endpoint
	///
	/// Put a sequence of frames onto the network
	template <typename FwdIterator>
	void send_frames(const identifier &ident, const FwdIterator first, const FwdIterator last)
	{
		typedef typename container_type::iterator iterator;
		iterator i = connections_.find(ident);
		assert(i != connections_.end());
		if (i != connections_.end()) {
			i->second->send_frames(first, last);
		}
	}
protected:
	typedef detail::solo_stream_service_impl<stream_type> impl_type;
	typedef boost::shared_ptr<impl_type>                  pimpl_type;

	identifier add_connection(const pimpl_type connection)
	{
		using boost::bind;
		using std::make_pair;
		identifier id;
		connections_.insert(make_pair(id, connection));
		connection->set_network_callback(bind(&basic_solo_stream::invoke_network_signal, this, _1, _2));
		return id;
	}

	void remove_connection(const identifier &id)
	{
		connections_.erase(id);
	}
private:
	typedef std::map<identifier, pimpl_type> container_type;

	container_type   connections_;
	network_signal_t network_signal_;

	void invoke_network_signal(const boost::system::error_code &error,
							   const identifier &id)
	{
		network_signal_(error, id);
	}
};     // class basic_solo_stream

/// \brief Actively open a TCP connection
///
/// Start a new connection by setting the remote endpoint
template <class StreamT>
class basic_solo_stream_initiator : public basic_solo_stream<StreamT, initiating_role> {
public:
	typedef StreamT                             stream_type;
	typedef stream_type&                        stream_reference;
	typedef boost::asio::io_service             service_type;
	typedef service_type&                       service_reference;
	typedef typename stream_type::endpoint_type endpoint_type;

	typedef basic_solo_stream<stream_type, initiating_role> super_type;
	
	basic_solo_stream_initiator(service_reference service)
		: super_type()
		, service_(service)
		, id_()
		, current_()
	{
	}

	virtual ~basic_solo_stream_initiator() {}

	/// \note should I change this to async_connect and accept a handler?
	void set_endpoint(const endpoint_type &ep)
	{
		using boost::bind;
		using boost::swap;
		using namespace boost::asio;

		pimpl_type next(new impl_type(service_));
		if (current_) {
			this->remove_connection(id_);
		}
		swap(current_, next);
		id_ = this->add_connection(current_);
		current_->get_stream().lowest_layer()
			.async_connect(ep,
						   bind(&impl_type::start,
								current_,
								placeholders::error,
								id_));
	}
private:
	typedef typename super_type::impl_type  impl_type;
	typedef typename super_type::pimpl_type pimpl_type;
	const service_reference service_;
	identifier              id_;
	pimpl_type              current_; // current network connection
};     // class basic_solo_stream_initiator

/// \brief Passively wait for a new TCP connection.
///
/// Listen for incoming connections
template <class StreamT>
class basic_solo_stream_listener : public basic_solo_stream<StreamT, listening_role> {
public:
	typedef StreamT                             stream_type;
	typedef stream_type&                        stream_reference;
	typedef boost::asio::io_service             service_type;
	typedef service_type&                       service_reference;
	typedef typename stream_type::protocol_type protocol_type;
	typedef typename protocol_type::acceptor    acceptor_type;
	typedef typename stream_type::endpoint_type endpoint_type;

	typedef basic_solo_stream<stream_type, listening_role> super_type;

	basic_solo_stream_listener(service_reference service)
		: super_type()
		, service_(service)
		, pimpl_(new listener_type(service))
		, conn_()
	{
		using boost::bind;
		conn_ =
			pimpl_->subscribe(bind(&basic_solo_stream_listener::handle_accept,
								   this, _1, _2));
	}

	basic_solo_stream_listener(service_reference svc, const endpoint_type &ep)
		: super_type()
		, service_(svc)
		, pimpl_(new listener_type(svc))
	{
		using boost::bind;
		conn_ =
			pimpl_->subscribe(bind(&basic_solo_stream_listener::handle_accept,
								   this, _1, _2));
		this->set_endpoint(ep);
	}

	virtual ~basic_solo_stream_listener()
	{
		conn_.disconnect();
	}

	void stop_listening()
	{
		pimpl_->close();
	}

	void set_endpoint(const endpoint_type &ep)
	{
		pimpl_->accept_from(ep);
		start_listening_connection();
	}
private:
	typedef typename super_type::impl_type       impl_type;
	typedef typename super_type::pimpl_type      pimpl_type;
	typedef detail::acceptor_impl<stream_type>   listener_type;
	typedef boost::shared_ptr<listener_type>     listener_pimpl;

	typedef typename listener_type::signal_connection_t signal_connection_t;

	const service_reference service_;
	listener_pimpl          pimpl_;
	signal_connection_t     conn_;

	void start_listening_connection()
	{
		pimpl_type next(new impl_type(service_));
		pimpl_->async_accept(next);
	}

	void handle_accept(const boost::system::error_code &error,
					   const pimpl_type next)
	{
		if (!error) {
			const identifier id = this->add_connection(next);
			next->start(error, id);
			start_listening_connection();
		}
	}
};     // class basic_solo_stream_listener
}      // namespace transport_service
}      // namespace beep
#endif // BEEP_SOLO_STREAM_HEAD
