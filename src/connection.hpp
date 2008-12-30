/// \file  connection.hpp
/// \brief BEEP TCP/IP connection on ASIO
#ifndef BEEP_CONNECTION_HEAD
#define BEEP_CONNECTION_HEAD 1
namespace beep {

namespace detail {

/// \brief Divide a message into frames
struct msg2frame {
	typedef list<frame>                           container;

	// should this take the transport layer as the initializer?
	msg2frame()
		: frames()
	{
	}

	void operator()(const channel &chan, const message &msg)
	{
		frames.clear();
		
		//frame nextFrame(const_buffer(msg.content(), msg.content_size()));
		frame nextFrame(msg);
		nextFrame.get_header().type = msg.type();
		nextFrame.get_header().channel = chan.number();
		nextFrame.get_header().msgno = chan.next_message_number();
		nextFrame.get_header().seqno = chan.next_sequence_number();
		//nextFrame.get_header().ansno = chan.next_answer_number();
		nextFrame.get_header().ansno = 0;

		frames.push_back(nextFrame);
	}

	container                 frames;
};     // struct frame_generator

template <typename T>
bool
scheduled_write_is_sentinel(const T &value)
{
	return value.first == numeric_limits<typename T::first_type>::max() &&
		value.second.empty();
}

template <class StreamType>
class basic_connection_impl
	: public enable_shared_from_this<basic_connection_impl<StreamType> >
{
public:
	typedef StreamType                                      stream_type;
	typedef shared_ptr<basic_connection_impl>               pointer;
	typedef enable_shared_from_this<basic_connection_impl>  base_type;
	typedef boost::system::error_code                       error_code;

	typedef void read_signature(const error_code&, size_t, frame::frame_type);
	typedef void write_signature(const error_code&, size_t);

	typedef function<read_signature>                        read_handler;
	typedef function<write_signature>                       write_handler;
	typedef pair<mutable_buffer, read_handler>              scheduled_read;
	typedef pair<message, write_handler>                    scheduled_write;

	basic_connection_impl(io_service &service)
		: base_type()
		, busywrite_(false)
		, busyread_(false)
		, drain_(false)
		, frame_()
		, stream_(service)
		, rsb_()
		, ssb_()
		, fssb_(&ssb_[0])
		, bssb_(&ssb_[1])
		, sched_()
		, writes_()
	{
	}

	virtual ~basic_connection_impl()
	{
		this->clear_pending_reads(boost::asio::error::operation_aborted);
		this->clear_pending_writes(boost::asio::error::operation_aborted);
	}

	stream_type &stream() { return stream_; }
	const stream_type &stream() const { return stream_; }

	void enqueue_read(const int channel, scheduled_read theRead)
	{
		assert(sched_.find(channel) == sched_.end());

		sched_.insert(make_pair(channel, theRead));
		if (!busyread_) {
			busyread_ = true;
			this->do_enqueue_read(bind(&basic_connection_impl::handle_frame_header,
									   this->shared_from_this(),
									   asio::placeholders::error,
									   asio::placeholders::bytes_transferred));
		}
	}

	void enqueue_write(const channel &chan, scheduled_write theWrite)
	{
		ostream bg(bssb_); // background stream

		detail::msg2frame gen;
		gen(chan, theWrite.first);

		size_t totalOctets = 0;
		typedef detail::msg2frame::container::const_iterator const_iterator;
		for (const_iterator i = gen.frames.begin(); i != gen.frames.end(); ++i){
			bg << i->get_header();
			for (frame::const_iterator j = i->begin(); j != i->end(); ++j) {
				if (bg.write(buffer_cast<const char*>(*j), buffer_size(*j))) {
					totalOctets += buffer_size(*j);
				} else {
					stream_.get_io_service()
						.dispatch(bind(theWrite.second,
									   boost::asio::error::no_memory, -1));
				}
			}
			bg << i->get_trailer();
		}
		//chan.increment_message_number();
		//chan.increase_sequence_number(totalOctets);

		writes_.push_back(make_pair(totalOctets, theWrite.second));
		// the write has been buffered, but not sent.
		this->enqueue_write();
	}

	void start()
	{
	}

	void stop()
	{
		drain_ = true;
		this->clear_pending_reads(boost::asio::error::operation_aborted);
		this->enqueue_write(); // enqueue in case there are no writes in progress
	}

private:
	typedef pair<size_t, write_handler>                     scheduled_write_proxy;
	typedef map<int, scheduled_read>                        read_container;
	typedef deque<scheduled_write_proxy>                    write_container;
	typedef asio::streambuf                                 streambuf_type;

	bool                      busywrite_;
	bool                      busyread_;
	bool                      drain_;
	frame                     frame_;    // current frame
	stream_type               stream_;   // socket
	streambuf_type            rsb_;      // streambuf for receiving data
	streambuf_type            ssb_[2];   // double buffered sends
	streambuf_type            *fssb_;    // streambuf that actively sends
	streambuf_type            *bssb_;    // background sending streambuf
	read_container            sched_;    // scheduled read buffers
	write_container           writes_;   // pending write handlers

	void terminate() { stream_.close(); }

	void
	handle_frame_header(const boost::system::error_code &error,
						size_t bytes_transferred)
	{
		if (!error || error == boost::asio::error::message_size) {
			if (!parse_frame_header(bytes_transferred, frame_.get_header())) {
				terminate();
			} else if (frame_.get_header().size > rsb_.size()) {
				const size_t remainingBytes =
					frame_.get_header().size - rsb_.size();
				this->do_enqueue_read(transfer_at_least(remainingBytes),
									  bind(&basic_connection_impl::handle_frame_payload,
										   this->shared_from_this(),
										   placeholders::error,
										   placeholders::bytes_transferred));
			} else {
				handle_frame_payload(error, rsb_.size());
			}
		} else {
			this->handle_stream_error(error);
		}
	}

	void
	handle_frame_payload(const boost::system::error_code &error,
						 size_t bytes_transferred)
	{
		const frame::header frameHeader(frame_.get_header());
		read_container::iterator i = sched_.find(frameHeader.channel);
		if (!error || error == boost::asio::error::message_size) {
			if (i != sched_.end()) {
				// i->second.first points to a boost ASIO mutable buffer
				rsb_.sgetn(buffer_cast<char*>(i->second.first), frameHeader.size);
			} else {
				// this is a problem!
				cerr << "There is no buffer ready for channel "
					 << frameHeader.channel << endl;
				rsb_.consume(frameHeader.size);
			}
			this->do_enqueue_read(bind(&basic_connection_impl::handle_frame_trailer,
									   this->shared_from_this(),
									   placeholders::error,
									   placeholders::bytes_transferred));
		} else {
			this->handle_stream_error(error);
		}
	}

	void
	handle_frame_trailer(const boost::system::error_code &error,
						 size_t bytes_transferred)
	{
		const frame::header frameHeader(frame_.get_header());
		read_container::iterator i = sched_.find(frameHeader.channel);
		if (!error || error == boost::asio::error::message_size) {
			if (!parse_frame_trailer(bytes_transferred, frame_.get_trailer())) {
				terminate();
			} else {
				const int chNum = frame_.get_header().channel;

				if (i != sched_.end()) {
					// i->second.second points to a read handler (callback function pointer)
					read_handler myHandler(i->second.second);
					sched_.erase(i);
					myHandler(error, frameHeader.size, frame::frame_type(frameHeader.type));
				}

				if (sched_.empty()) {
					busyread_ = false;
				} else {
					this->do_enqueue_read(bind(&basic_connection_impl::handle_frame_header,
											   this->shared_from_this(),
											   placeholders::error,
											   placeholders::bytes_transferred));
				}
			}
		} else {
			this->handle_stream_error(error);
		}
	}


	bool
	parse_frame_header(size_t bytes, frame::header &header)
	{
		return frame::parse(&rsb_, header);
	}

	bool
	parse_frame_trailer(size_t bytes, frame::trailer &trailer)
	{
		return frame::parse(&rsb_, trailer);
	}

	// schedule to flush the current send buffer over the socket
	void
	enqueue_write()
	{
		stream_.get_io_service()
			.post(bind(&basic_connection_impl::do_enqueue_write,
					   this->shared_from_this()));
	}

	void
	do_unsafe_enqueue_write()
	{
		if (!busywrite_) {
			busywrite_ = true;
			// no active write operation, so enque a new one.
			swap(bssb_, fssb_);
			// insert a sentinel
			writes_.push_back(make_pair(numeric_limits<scheduled_write_proxy::first_type>::max(),
										write_handler()));
			async_write(stream_, *fssb_,
						bind(&basic_connection_impl::handle_send,
							 this->shared_from_this(),
							 placeholders::error,
							 placeholders::bytes_transferred));
		} else if (drain_) {
			stream_.lowest_layer().shutdown(stream_type::lowest_layer_type::shutdown_send);
			busywrite_ = false;
		}
	}

	void
	do_enqueue_write()
	{
		try {
			this->do_unsafe_enqueue_write();
		} catch (const boost::system::system_error &ex) {
			this->handle_stream_error(ex.code());
		}
	}

	template <class Handler>
	void
	do_enqueue_read(Handler handler)
	{
		try {
			async_read_until(stream_, rsb_, frame::terminator(), handler);
		} catch (const boost::system::system_error &ex) {
			this->handle_stream_error(ex.code());
		}
	}

	template <class T, class Handler>
	void
	do_enqueue_read(T CompletionCondition, Handler handler)
	{
		try {
			async_read(stream_, rsb_, CompletionCondition, handler);
		} catch (const boost::system::system_error &ex) {
			this->handle_stream_error(ex.code());
		}
	}

	void
	handle_send(const boost::system::error_code &error,
				size_t bytes_transferred)
	{
		if (!error || error == boost::asio::error::message_size) {
			// invoke the pending write handlers now.
			bool done = false;
			while (!writes_.empty() && !done) {
				scheduled_write_proxy next_handler = writes_.front();
				done = detail::scheduled_write_is_sentinel(next_handler);
				if (!done) {
					const size_t bytes_in_send = next_handler.first;
					assert(!next_handler.second.empty());
					next_handler.second(error, bytes_in_send);
				}
				writes_.pop_front();
			}

			busywrite_ = false;
			if (bssb_->size()) {
				do_enqueue_write();
			}
		} else {
			this->handle_stream_error(error);
		}
	}

	void
	handle_stream_error(const boost::system::error_code &error)
	{
		this->clear_pending_reads(error);
		this->clear_pending_writes(error);
	}

	void
	clear_pending_reads(const boost::system::error_code &error)
	{
		// tell all queued readers of the read problem.
		typedef typename read_container::iterator iterator;
		for (iterator i = sched_.begin(); i != sched_.end(); ++i) {
			i->second.second(error, 0, frame::err);
		}
		sched_.clear();
		busyread_ = false;
	}

	void
	clear_pending_writes(const boost::system::error_code &error)
	{
		// invoke all of the pending write handlers.
		while (!writes_.empty()) {
			scheduled_write_proxy next_handler = writes_.front();
			if (!detail::scheduled_write_is_sentinel(next_handler)) {
				next_handler.second(error, 0);
			}
			writes_.pop_front();
		}

		// clear out the send buffers
		while (const size_t theSize = fssb_->size()) {
			fssb_->consume(theSize);
		}
		while (const size_t theSize = bssb_->size()) {
			bssb_->consume(theSize);
		}
		busywrite_ = false;
	}
};

}      // namespace detail

template <class StreamType>
class basic_connection : private noncopyable {
public:
	typedef StreamType                                      stream_type;
	typedef stream_type                                     next_layer_type;
	typedef typename next_layer_type::lowest_layer_type     lowest_layer_type;

	basic_connection(io_service &service)
		: pimpl_(new impl_type(service))
	{
	}

	virtual ~basic_connection()
	{
	}

	next_layer_type &next_layer() { return pimpl_->stream(); }
	lowest_layer_type &lowest_layer() { return pimpl_->stream().lowest_layer(); }

	template <class MutableBuffer, class Handler>
	void read(const channel &chan, MutableBuffer buffer, Handler handler)
	{
		pimpl_->enqueue_read(chan.number(), make_pair(buffer, handler));
	}

	template <class ConstBuffer, class Handler>
	void send(channel &chan, ConstBuffer buffer, Handler handler)
	{
		pimpl_->enqueue_write(chan, make_pair(buffer, handler));
	}

	void start()
	{
		pimpl_->start();
	}

	void stop()
	{
		pimpl_->stop();
	}

private:
	typedef detail::basic_connection_impl<next_layer_type>  impl_type;
	typedef typename impl_type::pointer                              pimpl_type;

	pimpl_type                pimpl_;
};     // class basic_connection

}      // namespace beep
#endif // BEEP_CONNECTION_HEAD
