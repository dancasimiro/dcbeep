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

	template <class T>
	void operator()(const T &channel, const message &msg)
	{
		frames.clear();
		
		//frame nextFrame(const_buffer(msg.content(), msg.content_size()));
		frame nextFrame(msg);
		nextFrame.get_header().type = msg.type();
		nextFrame.get_header().channel = channel.number();
		nextFrame.get_header().msgno = channel.next_message_number();
		nextFrame.get_header().seqno = channel.next_sequence_number();
		//nextFrame.get_header().ansno = channel.next_answer_number();
		nextFrame.get_header().ansno = 0;

		frames.push_back(nextFrame);
	}

	container                 frames;
};     // struct frame_generator

}      // namespace detail

template <class StreamType>
class basic_connection : private noncopyable {
public:
	typedef StreamType                                      stream_type;
	typedef stream_type                                     next_layer_type;
	typedef typename next_layer_type::lowest_layer_type     lowest_layer_type;
	typedef boost::system::error_code                       error_code;
	typedef function<void (error_code, size_t)>             data_callback;

	basic_connection(io_service &service)
		: stream_(service)
		, rsb_()
		, frame_()
		, ssb_()
		, fssb_(&ssb_[0])
		, bssb_(&ssb_[1])
		, bufs_()
		, rcbs_()
		, scbs_()
		, started_(false)
	{
	}

	next_layer_type &next_layer() { return stream_; }
	lowest_layer_type &lowest_layer() { return stream_.lowest_layer(); }

	template <class Channel, class MutableBuffer, class Handler>
	void async_read(Channel &chan, MutableBuffer buffer, Handler handler)
	{
		const int chNum = chan.number();
		assert(bufs_.find(chNum) == bufs_.end());
		assert(rcbs_.find(chNum) == rcbs_.end());

		bufs_.insert(make_pair(chNum, buffer));
		rcbs_.insert(make_pair(chNum, handler));

		async_read_until(stream_, rsb_,
						 frame::terminator(),
						 bind(&basic_connection::handle_frame_header, this,
							  placeholders::error,
							  placeholders::bytes_transferred));
	}

	template <class Channel, class ConstBuffer, class Handler>
	void async_send(Channel &chan, ConstBuffer buffer, Handler handler)
	{
		const int chNum = chan.number();
		assert(scbs_.find(chNum) == scbs_.end());

		ostream bg(bssb_); // background stream

		detail::msg2frame gen;
		gen(chan, buffer);

		size_t totalOctets = 0;
		typedef detail::msg2frame::container::const_iterator const_iterator;
		for (const_iterator i = gen.frames.begin(); i != gen.frames.end(); ++i){
			bg << i->get_header();
			for (frame::const_iterator j = i->begin(); j != i->end(); ++j) {
				if (bg.write(buffer_cast<const char*>(*j), buffer_size(*j))) {
					totalOctets += buffer_size(*j);
				} else {
					// failure!!!
					assert(false);
				}
			}
			bg << i->get_trailer();
		}
		chan.increment_message_number();
		chan.increase_sequence_number(totalOctets);

		// the write has been buffered, but not sent.
		boost::system::error_code error;
		stream_.get_io_service().dispatch(bind(handler, error, totalOctets));
		this->enqueue_write();
	}

	void start()
	{
		started_ = true;
	}
private:
	typedef map<int, data_callback>     callback_container;
	typedef map<int, mutable_buffer>    buffers_container;

	next_layer_type           stream_;   // socket
	asio::streambuf           rsb_;      // streambuf for receiving data
	frame                     frame_;    // current frame
	asio::streambuf           ssb_[2];   // double buffered sends
	asio::streambuf           *fssb_;    // streambuf that actively sends
	asio::streambuf           *bssb_;    // background sending streambuf
	buffers_container         bufs_;     // read buffers for each channel
	callback_container        rcbs_;     // read callbacks
	callback_container        scbs_;     // send callbacks
	bool                      started_;

	void
	handle_frame_header(const boost::system::error_code &error,
						size_t bytes_transferred)
	{
		if (!error || error == boost::asio::error::message_size) {
			if (!parse_frame_header(bytes_transferred, frame_.get_header())) {
				terminate_connection();
			} else if (frame_.get_header().size > rsb_.size()) {
				const size_t remainingBytes =
					frame_.get_header().size - rsb_.size();
				::async_read(stream_, rsb_,
							 transfer_at_least(remainingBytes),
							 bind(&basic_connection::handle_frame_payload, this,
								  placeholders::error,
								  placeholders::bytes_transferred));
			} else {
				handle_frame_payload(error, rsb_.size());
			}
		} else {
			cerr << "Bad Frame Header: " << error.message() << endl;
		}
	}

	void
	handle_frame_payload(const boost::system::error_code &error,
						 size_t bytes_transferred)
	{
		if (!error || error == boost::asio::error::message_size) {
			const frame::header frameHeader(frame_.get_header());
			buffers_container::iterator i = bufs_.find(frameHeader.channel);
			if (i != bufs_.end()) {
				rsb_.sgetn(buffer_cast<char*>(bufs_[frameHeader.channel]),
						   frameHeader.size);
			} else {
				cout << "There is no buffer ready for channel "
					 << frameHeader.channel << endl;
				rsb_.consume(frameHeader.size);
			}

			async_read_until(stream_, rsb_,
							 frame::terminator(),
							 bind(&basic_connection::handle_frame_trailer, this,
								  placeholders::error,
								  placeholders::bytes_transferred));
		} else {
			cerr << "Bad Frame Payload: " << error.message() << endl;
		}
	}

	void
	handle_frame_trailer(const boost::system::error_code &error,
						 size_t bytes_transferred)
	{
		if (!error || error == boost::asio::error::message_size) {
			if (!parse_frame_trailer(bytes_transferred, frame_.get_trailer())) {
				terminate_connection();
			} else {
				const int chNum = frame_.get_header().channel;
				data_callback myCallback(rcbs_[chNum]);
				bufs_.erase(chNum);
				rcbs_.erase(chNum);
				
				boost::system::error_code noError;
				myCallback(noError, frame_.get_header().size);

				cout << frame_.get_trailer() << "\n*********" << endl;
				cout << "RECV END." << endl;
			}
		} else {
			cerr << "Bad Frame Trailer: " << error.message() << endl;
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

	void
	terminate_connection()
	{
		stream_.close();
	}

	// schedule to flush the current send buffer over the socket
	void
	enqueue_write()
	{
		stream_.get_io_service()
			.post(bind(&basic_connection::do_enqueue_write, this));
	}

	void
	do_enqueue_write()
	{
		if (!started_ || fssb_->size() == 0) {
			// no active write operation, so enque a new one.
			swap(bssb_, fssb_);
			async_write(stream_, *fssb_,
						bind(&basic_connection::handle_send,
							 this,
							 placeholders::error,
							 placeholders::bytes_transferred));
		}
	}

	void
	handle_send(const boost::system::error_code &error,
				size_t bytes_transferred)
	{
		if (!error || error == boost::asio::error::message_size) {
			if (bssb_->size()) {
				do_enqueue_write();
			}
		} else {
			cerr << "Incomplete send: " << error.message()
				 << " code is " << error.value();
		}
	}
};     // class basic_connection

}      // namespace beep
#endif // BEEP_CONNECTION_HEAD
