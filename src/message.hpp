/// \file  message.hpp
/// \brief BEEP message
///
/// UNCLASSIFIED
#ifndef BEEP_MESSAGE_HEAD
#define BEEP_MESSAGE_HEAD 1
namespace beep {

class message {
public:
	typedef deque<const_buffer>                   buffers_container;
	typedef buffers_container::const_iterator     const_iterator;

	enum frame_type {
		msg = 0,
		rpy = 1,
		ans = 2,
		err = 3,
		nul = 4,
	};

	message()
		: content_()
		, type_(msg)
		, chnno_(0)
		, msgno_(0)
		, seqno_(0)
		, ansno_(0)
	{
	}

	virtual ~message()
	{
	}
	
	uint32_t channel_number() const { return chnno_; }
	uint32_t message_number() const { return msgno_; }
	uint32_t sequence_number() const { return seqno_; }
	uint32_t answer_number() const { return ansno_; }
	int      type() const { return type_; }

	void set_channel_number(const uint32_t chnum) { chnno_ = chnum; }
	void set_message_number(const uint32_t msgno) { msgno_ = msgno; }
	void set_sequence_number(const uint32_t seqno) { seqno_ = seqno; }
	void set_answer_number(const uint32_t ansno) { ansno_ = ansno; }
	void set_type(const int t) { type_ = t; }

	const_iterator begin() const { return content_.begin(); }
	const_iterator end() const { return content_.end(); }

	void add_header(const_buffer ehbuf)
	{
		content_.push_front(ehbuf);
		if (content_.size() == 1) {
			content_.push_back(buffer("\r\n"));
		}
	}

	void add_content(const_buffer buf)
	{
		content_.push_back(buf);
	}
private:
	buffers_container         content_;
	int                       type_;

	uint32_t                  chnno_, msgno_, seqno_, ansno_;
};     // class message

}      // namespace beep
#endif // BEEP_MESSAGE_HEAD
