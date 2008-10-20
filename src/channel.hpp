// \file  channel.hpp
/// \brief BEEP channel built on Boost ASIO
///
/// UNCLASSIFIED
#ifndef BEEP_CHANNEL_HEAD
#define BEEP_CHANNEL_HEAD 1
namespace beep {

namespace detail {

/// \brief Divide a message into frames
struct message_splitter {
	typedef list<frame>                           frames_container;

	// should this take the transport layer as the initializer?
	message_splitter()
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

		nextFrame.refresh();
		frames.push_back(nextFrame);
	}

	frames_container          frames;
};     // struct frame_generator

template <class T>
asio::streambuf *connection_send_streambuf(T &connection)
{
	return connection.bssb_;
}

}      // namespace detail

/// \brief Establish a new channel in the BEEP session
template <class SessionType>
class basic_channel {
public:
	typedef SessionType                                   session_type;
	typedef session_type&                                 session_reference;
	typedef shared_ptr<profile>                           profile_pointer;
	typedef typename session_type::connection_type        connection_type;
	typedef typename session_type::connection_reference   connection_reference;

	basic_channel()
		: num_(0)
		, msgno_(0)
		, seqno_(0)
		, profile_()
	{
	}

	basic_channel(session_reference mySession)
		: num_(mySession.next_channel_number())
		, msgno_(0)
		, seqno_(0)
		, profile_()
	{
	}

	basic_channel(const basic_channel& src)
		: num_(src.num_)
		, msgno_(src.msgno_)
		, seqno_(src.seqno_)
		, profile_(src.profile_)
	{
	}

	basic_channel &operator=(const basic_channel &src)
	{
		if (&src != this) {
			this->num_ = src.num_;
			this->msgno_ = src.msgno_;
			this->seqno_ = src.seqno_;
			this->profile_ = src.profile_;
		}
		return *this;
	}

	void set_profile(profile_pointer pp)
	{
		profile_ = pp;
	}
	profile_pointer get_profile() { return profile_; }

	uint32_t number() const { return num_; }
	void set_number(uint32_t num) { num_ = num; }
	uint32_t next_message_number() const { return msgno_; }
	uint32_t next_sequence_number() const { return seqno_; }
private:
	void increment_message_number() { ++msgno_; }
	void increase_sequence_number(const size_t octs) { seqno_ += octs; }

private:
	uint32_t                  num_, msgno_, seqno_;
	profile_pointer           profile_;  // active profile
public:
	void encode(message &msg, connection_reference connection)
	{
		list<frame> frames;
		detail::message_splitter frame_maker;
		frame_maker(*this, msg);

		typedef detail::message_splitter::frames_container::const_iterator
			const_iterator;

		size_t totalOctets = 0;
		// for each frame
		for (const_iterator i = frame_maker.frames.begin();
			 i != frame_maker.frames.end(); ++i) {

			// for each buffer within the frame
			typedef vector<const_buffer>::const_iterator buf_const_iter;
			asio::streambuf * const bssb =
				detail::connection_send_streambuf(connection);
			for (buf_const_iter j = i->begin(); j != i->end(); ++j) {
				bssb->sputn(buffer_cast<const char*>(*j),
							buffer_size(*j));
				totalOctets += buffer_size(*j);
			}
		}
		this->increment_message_number();
		this->increase_sequence_number(totalOctets);
	}
};     // class basic_channel  

}      // namespace beep
#endif // BEEP_CHANNEL_HEAD
