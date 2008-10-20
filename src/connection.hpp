/// \file  connection.hpp
/// \brief BEEP TCP/IP connection on ASIO
#ifndef BEEP_CONNECTION_HEAD
#define BEEP_CONNECTION_HEAD 1
namespace beep {

namespace detail {

void noop_message_cb(const message&) { }

}      // namespace detail

template <class StreamType>
class basic_connection : private noncopyable {
public:
	typedef StreamType                                      stream_type;
	typedef stream_type                                     next_layer_type;
	typedef typename next_layer_type::lowest_layer_type     lowest_layer_type;
	typedef function<void (message)>                        message_callback;

	template <class T> friend
	asio::streambuf* detail::connection_send_streambuf(T&);

	basic_connection(io_service &service)
		: stream_(service)
		, rsb_()
		, frame_()
		, framebuf_()
		, callback_(detail::noop_message_cb)
		, ssb_()
		, fssb_(&ssb_[0])
		, bssb_(&ssb_[1])
	{
	}

	next_layer_type &next_layer() { return stream_; }
	lowest_layer_type &lowest_layer() { return stream_.lowest_layer(); }

	void install_message_callback(message_callback cb) { callback_ = cb; }

	void start()
	{
		async_read_until(stream_, rsb_,
						 frame::terminator(),
						 bind(&basic_connection::handle_frame_header, this,
							  placeholders::error,
							  placeholders::bytes_transferred));
	}

	void transmit()
	{
		// create the frames from the message
		// Encode the frame data and copy to the sending streambuf
		// schedule the transmission

		// 1. break channel, msg into frames
		// 2. push frames into the stream buffer
		// 3. enqueue the write.
		this->enqueue_write();
	}

private:
	next_layer_type           stream_;   // socket
	asio::streambuf           rsb_;      // streambuf for receiving data
	frame                     frame_;    // current frame
	vector<char>              framebuf_; // buffer used to store split messages
	message_callback          callback_; // handle incoming messages
	asio::streambuf           ssb_[2];   // double buffered sends
	asio::streambuf           *fssb_;    // streambuf that actively sends
	asio::streambuf           *bssb_;    // background sending streambuf

	void
	handle_frame_header(const boost::system::error_code &error,
						size_t bytes_transferred)
	{
		if (!error || error == boost::asio::error::message_size) {
			frame::header myHead;
			if (!parse_frame_header(bytes_transferred, myHead)) {
				terminate_connection();
			} else if (myHead.size > rsb_.size()) {
				const size_t remainingBytes = myHead.size - rsb_.size();
				cout << "Read more data: " << remainingBytes << endl;
				cout << "RSB Size is " << rsb_.size() << endl;
				cout << "Frame Payload Size is " << myHead.size << endl;
				frame_.set_header(myHead);
				async_read(stream_, rsb_,
						   transfer_at_least(remainingBytes),
						   bind(&basic_connection::handle_frame_payload, this,
								placeholders::error,
								placeholders::bytes_transferred));
			} else {
				cout << "Invoke payload handler directly!" << endl;
				frame_.set_header(myHead);
				handle_frame_payload(error, rsb_.size());
			}
			//cout << "Message Number is " << frame_.get_header().msgno << endl;
			//cout << "Message Channel is " << frame_.get_header().channel << endl;
		} else {
			cerr << "Bad Frame Header: " << error.message() << endl;
		}
	}

	void
	handle_frame_payload(const boost::system::error_code &error,
						 size_t bytes_transferred)
	{
		cout << "handle_frame_payload " << bytes_transferred << endl;
		if (!error || error == boost::asio::error::message_size) {
			const frame::header frameHeader(frame_.get_header());
			cout << "Frame Header size == " << frameHeader.size << endl;
			framebuf_.resize(frameHeader.size + 1, '\0');
			rsb_.sgetn(&framebuf_[0], frameHeader.size);

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
				// create a message from the payload...
				message myMessage;
#if 1
				myMessage.set_channel_number(frame_.get_header().channel);
				myMessage.set_message_number(frame_.get_header().msgno);
				myMessage.set_sequence_number(frame_.get_header().seqno);
				myMessage.set_type(frame_.get_header().type);
				myMessage.add_content(buffer(framebuf_));
#endif
				cout << "Invoke message callback for:\n " << &framebuf_[0] << endl;
				callback_(myMessage);

				async_read_until(stream_, rsb_,
								 frame::terminator(),
								 bind(&basic_connection::handle_frame_header,
									  this,
									  placeholders::error,
									  placeholders::bytes_transferred));
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
		if (fssb_->size() == 0) {
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
