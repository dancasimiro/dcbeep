/// \file  frame.hpp
/// \brief BEEP frame
///
/// UNCLASSIFIED
#ifndef BEEP_FRAME_HEAD
#define BEEP_FRAME_HEAD 1

#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>

namespace beep {

enum core_message_types {
	MSG = 0,
	RPY = 1,
	ANS = 2,
	ERR = 3,
	NUL = 4,
};

class frame {
public:
	template <typename Iterator> friend struct frame_parser;
	typedef std::string string_type;

	static const string_type &sentinel()
	{
		static const string_type sent("END\r\n");
		return sent;
	}

	static const string_type &terminator()
	{
		static const string_type term("\r\n");
		return term;
	}

	static char separator()
	{
		static const char s = ' ';
		return s;
	}
public:
	frame()
		: header_(MSG)
		, payload_()
		, channel_(0)
		, message_(0)
		, sequence_(0)
		, answer_(0)
		, more_(false)
	{
	}

	const unsigned int get_type() const { return header_; }
	const unsigned int get_channel() const { return channel_; }
	const unsigned int get_message() const { return message_; }
	const bool get_more() const { return more_; }
	const unsigned int get_sequence() const { return sequence_; }
	const string_type &get_payload() const { return payload_; }
	const unsigned int get_answer() const { return answer_; }

	void set_type(const unsigned int h) { header_ = h; }
	void set_channel(const unsigned int c) { channel_ = c; }
	void set_message(const unsigned int m) { message_ = m; }
	void set_more(const bool m) { more_ = m; }
	void set_sequence(const unsigned int s) { sequence_ = s; }
	void set_payload(const string_type &p) { payload_ = p; }
	void set_payload_vector(const std::vector<char> &v) { payload_.assign(v.begin(), v.end()); }
	void set_answer(const unsigned int a) { answer_ = a; }
private:
	unsigned int header_;
	string_type  payload_;
	unsigned int channel_;
	unsigned int message_;
	unsigned int sequence_;
	unsigned int answer_;
	bool         more_;
};     // class frame

inline
bool operator==(const frame &lhs, const frame &rhs)
{
	return lhs.get_type() == rhs.get_type() &&
		lhs.get_channel() == rhs.get_channel() &&
		lhs.get_message() == rhs.get_message() &&
		lhs.get_more() == rhs.get_more() &&
		lhs.get_sequence() == rhs.get_sequence() &&
		lhs.get_answer() == rhs.get_answer() &&
		lhs.get_payload() == rhs.get_payload();
}
}      // namespace beep
#endif // BEEP_FRAME_HEAD
