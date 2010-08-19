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
#include <iterator>
#include <map>
#include <limits>

#include <boost/noncopyable.hpp>
#include <boost/throw_exception.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/signals2.hpp>
#include <boost/date_time.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include "beep/role.hpp"
#include "beep/frame.hpp"
#include "beep/frame-parser.hpp"
#include "beep/frame-stream.hpp"
#include "beep/reply-code.hpp"

namespace beep {
typedef boost::uuids::uuid identifier;

namespace transport_service {
namespace detail {

enum seq_frame_identifier {
	is_data_frame = 0,
	is_seq_frame = 1,
};

class frame_handler_visitor : public boost::static_visitor<seq_frame_identifier> {
public:
	seq_frame_identifier operator()(const seq_frame &) const
	{
		return is_seq_frame;
	}

	template <typename T>
	seq_frame_identifier operator()(const T &) const
	{
		return is_data_frame;
	}
};     // class frame_handler_visitor

class channel_number_visitor : public boost::static_visitor<unsigned int> {
public:
	template <typename T>
	unsigned int operator()(const T &frm) const
	{
		return frm.channel;
	}
};     // class channel_number_visitor

class window_size_visitor : public boost::static_visitor<unsigned int> {
public:
	template <typename T>
	unsigned int operator()(const T &) const
	{
		return 0;
	}

	unsigned int operator()(const seq_frame &frm) const
	{
		return frm.window;
	}
};     // class window_size_visitor

class acknowledgement_visitor : public boost::static_visitor<unsigned int> {
public:
	template <typename T>
	unsigned int operator()(const T &frm) const
	{
		return frm.sequence + frm.payload.size();
	}

	unsigned int operator()(const seq_frame &) const
	{
		return 0;
	}
};     // class acknowledgement_number_visitor

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

	typedef beep::identifier identifier;

	typedef boost::signals2::connection signal_connection;
	typedef boost::system::error_code   error_code_type;
	typedef boost::signals2::signal<void (const error_code_type&, const frame&)> frame_signal_t;

	typedef boost::function<void (const error_code_type&, const identifier&)> network_cb_t;

	solo_stream_service_impl(service_reference service)
		: stream_(service)
		, timer_(service)
		, rsb_()
		, wsb0_()
		, wsb1_()
		, fwsb_(&wsb0_)
		, bwsb_(&wsb1_)
		, wstrand_(service)
		, signal_frame_()
		, net_changed_()
		, peer_window_size_(4096u)
	{
	}

	solo_stream_service_impl(service_reference service, const std::size_t maxSize)
		: stream_(service)
		, timer_(service)
		, rsb_(maxSize)
		, wsb0_(maxSize)
		, wsb1_(maxSize)
		, fwsb_(&wsb0_)
		, bwsb_(&wsb1_)
		, wstrand_(service)
		, signal_frame_()
		, net_changed_()
		, peer_window_size_(4096u)
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
	typedef boost::asio::deadline_timer timer_type;
	typedef boost::asio::streambuf streambuf_type;
	typedef service_type::strand   strand_type;
	typedef size_t size_type;

	stream_type    stream_;
	timer_type     timer_;
	streambuf_type rsb_; // read streambuf
	streambuf_type wsb0_, wsb1_; // double buffered write streambufs
	streambuf_type *fwsb_;  // current (foreground) sendbuf that is being sent
	streambuf_type *bwsb_;  // background streambuf that is updated while fwsb_ is transmitted
	strand_type    wstrand_; // serialize write operations
	frame_signal_t signal_frame_;
	network_cb_t   net_changed_;
	size_type      peer_window_size_;

	static boost::posix_time::time_duration get_response_timeout()
	{
		return boost::posix_time::minutes(5);
	}

	void set_error(const boost::system::error_code &error)
	{
		boost::system::error_code close_error;
		stream_.close(close_error);
		fwsb_->consume(fwsb_->size());
		bwsb_->consume(bwsb_->size());
		timer_.cancel();
		signal_frame_(error, beep::frame());
		if (close_error) {
			signal_frame_(close_error, beep::frame());
		}
	}

	void serialize_frame(const frame &frame)
	{
		using std::ostream;
		/// \todo split frame into multiple frames if it is larger than the advertized SEQ window
		/// \todo implement flow control?
		ostream stream(bwsb_);
		if (!(stream << frame)) {
			using namespace boost::system::errc;
			// I'm guessing the operator<< failed because the bwsb_ is too small.
			// I'm not sure how to get access to the actual error condition.
			set_error(make_error_code(value_too_large));
		}
	}

	void do_send_if_possible()
	{
		using boost::bind;
		if (fwsb_->size() == 0 && stream_.lowest_layer().is_open() && bwsb_->size() > 0) {
			do_send();
			/// Time out if the peer does not send a response
			timer_.cancel();
			timer_.expires_from_now(get_response_timeout());
			timer_.async_wait(bind(&solo_stream_service_impl::handle_data_timeout,
								   this->shared_from_this(),
								   boost::asio::placeholders::error));
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
						 sentinel(),
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
		using std::ostream;
		timer_.cancel();
		if (!error || error == boost::asio::error::message_size) {
			std::vector<frame> current_frames;
			istream stream(&rsb_);
			parse_frames(stream, current_frames);
			std::size_t num_frames = 0;
			for (std::vector<frame>::const_iterator i = current_frames.begin(); i != current_frames.end(); ++i) {
				const beep::frame current = *i;
				if (apply_visitor(frame_handler_visitor(), current) == is_seq_frame) {
					peer_window_size_ = apply_visitor(window_size_visitor(), current);
				} else {
					num_frames += 1u;
					signal_frame_(boost::system::error_code(), current);
					// send back a SEQ frame advertising the new window
					seq_frame new_window_ad;
					new_window_ad.channel = apply_visitor(channel_number_visitor(), current);
					new_window_ad.acknowledgement = apply_visitor(acknowledgement_visitor(), current);
					new_window_ad.window = 4096u;
					send_frame(new_window_ad);
				}
			}
			if (num_frames) {
				do_start_read();
			} else {
				// There must be at least one valid parsed frame because I read
				// until there was at least one END\r\n sequence in the streambuf.
				// There must be a syntax error if I did not parse at least one data
				// frame correctly. Subsequent errors could be due to incomplete frames
				// sitting in the streambuf.
				set_error(boost::system::error_code(beep::reply_code::general_syntax_error,
													beep::beep_category));
			}
		} else {
			set_error(error);
		}
	}

	void handle_data_timeout(const boost::system::error_code &error)
	{
		if (!error) {
			using namespace boost::system::errc;
			set_error(make_error_code(timed_out));
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

	void cancel()
	{
		acceptor_.cancel();
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

	typedef detail::solo_stream_service_impl<stream_type> impl_type;
	typedef typename impl_type::identifier identifier;

	typedef boost::signals2::signal<void (const boost::system::error_code&, const identifier&)> network_signal_t;
	typedef boost::signals2::signal<void (const boost::system::error_code&, const frame&)>      frame_signal_t;

	static role get_role() { return RoleEnum; }

	basic_solo_stream()
		: connections_()
		, network_signal_()
	{
	}

	virtual ~basic_solo_stream() {}

	void shutdown_connection(const identifier &ident)
	{
		typedef typename container_type::iterator iterator;
		iterator i = connections_.find(ident);
		assert(i != connections_.end());
		if (i != connections_.end()) {
			i->second->get_stream().shutdown(stream_type::shutdown_send);
		}
	}

	// closes the associated socket and removes the connection object
	void stop_connection(const identifier &ident)
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
	signal_connection install_network_handler(const typename network_signal_t::slot_type slot)
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
	typedef boost::shared_ptr<impl_type>                  pimpl_type;

	identifier add_connection(const pimpl_type connection)
	{
		using boost::bind;
		using std::make_pair;
		identifier id = boost::uuids::random_generator()();
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
	typedef typename super_type::identifier identifier;
	
	basic_solo_stream_initiator(service_reference service, const std::size_t maxbufsz = std::numeric_limits<std::size_t>::max())
		: super_type()
		, service_(service)
		, id_()
		, current_()
		, maxbufsz_(maxbufsz)
	{
	}

	virtual ~basic_solo_stream_initiator() {}

	/// \note should I change this to async_connect and accept a handler?
	void set_endpoint(const endpoint_type &ep)
	{
		using boost::bind;
		using boost::swap;
		using namespace boost::asio;

		pimpl_type next(new impl_type(service_, maxbufsz_));
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
	size_t                  maxbufsz_;
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
	typedef typename super_type::identifier identifier;

	basic_solo_stream_listener(service_reference service, const std::size_t maxbufsz = std::numeric_limits<std::size_t>::max())
		: super_type()
		, service_(service)
		, pimpl_(new listener_type(service))
		, conn_()
		, maxbufsz_(maxbufsz)
	{
		using boost::bind;
		conn_ =
			pimpl_->subscribe(bind(&basic_solo_stream_listener::handle_accept,
								   this, _1, _2));
	}

	basic_solo_stream_listener(service_reference svc, const endpoint_type &ep, const std::size_t maxbufsz = std::numeric_limits<std::size_t>::max())
		: super_type()
		, service_(svc)
		, pimpl_(new listener_type(svc))
		, maxbufsz_(maxbufsz)
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
		stop_listening();
	}

	void stop_listening()
	{
		pimpl_->cancel();
	}

	void start_listening()
	{
		// next is pointer to the connection implementation
		pimpl_type next(new impl_type(service_, maxbufsz_));
		// pimpl_ is a pointer to the listening implementation
		pimpl_->async_accept(next);
	}

	void set_endpoint(const endpoint_type &ep)
	{
		pimpl_->accept_from(ep);
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
	std::size_t             maxbufsz_;

	void handle_accept(const boost::system::error_code &error,
					   const pimpl_type next)
	{
		if (!error) {
			const identifier id = this->add_connection(next);
			next->start(error, id);
			start_listening();
		}
	}
};     // class basic_solo_stream_listener
}      // namespace transport_service
}      // namespace beep
#endif // BEEP_SOLO_STREAM_HEAD
