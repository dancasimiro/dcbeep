/// \file  message.hpp
/// \brief BEEP message
///
/// UNCLASSIFIED
#ifndef BEEP_MESSAGE_HEAD
#define BEEP_MESSAGE_HEAD 1

//#include <vector>
#include <string>

namespace beep {

class basic_message {
public:
	typedef std::string header_type;

	uint32_t channel_number() const { return chnno_; }
	uint32_t message_number() const { return msgno_; }
	uint32_t sequence_number() const { return seqno_; }
	uint32_t answer_number() const { return ansno_; }

	void set_channel_number(const uint32_t chnum) { chnno_ = chnum; }
	void set_message_number(const uint32_t msgno) { msgno_ = msgno; }
	void set_sequence_number(const uint32_t seqno) { seqno_ = seqno; }
	void set_answer_number(const uint32_t ansno) { ansno_ = ansno; }

protected:
	basic_message(const header_type &h)
		: type_(h)
		, chnno_(0)
		, msgno_(0)
		, seqno_(0)
		, ansno_(0)
	{
	}

	virtual ~basic_message()
	{
	}
private:
	const header_type type_;
	unsigned int      chnno_, msgno_, seqno_, ansno_;
};     // class basic_message

class message : public basic_message {
public:
	message() : basic_message("MSG") {}
	virtual ~message() {}
};     // class message

class reply : public basic_message {
public:
	reply() : basic_message("RPY") {}
	virtual ~reply() {}
};     // class reply

}      // namespace beep
#endif // BEEP_MESSAGE_HEAD
