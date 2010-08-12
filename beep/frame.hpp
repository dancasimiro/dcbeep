/// \file  frame.hpp
/// \brief BEEP frame
///
/// UNCLASSIFIED
#ifndef BEEP_FRAME_HEAD
#define BEEP_FRAME_HEAD 1

#include <string>
#include <boost/variant.hpp>

namespace beep {

enum core_message_types {
	MSG = 0,
	RPY = 1,
	ANS = 2,
	ERR = 3,
	NUL = 4,
	SEQ = 5,
};

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

template <core_message_types CoreFrameType>
struct basic_frame {
	basic_frame()
		: payload()
		, channel(0)
		, message(0)
		, sequence(0)
		, more(false)
	{
	}

	static const core_message_types header() { return CoreFrameType; }

	string_type  payload;
	unsigned int channel;
	unsigned int message;
	unsigned int sequence;
	bool         more;

	bool
	operator==(const basic_frame &rhs) const
	{
		return this->header() == rhs.header() &&
			this->channel == rhs.channel &&
			this->message == rhs.message &&
			this->more == rhs.more &&
			this->sequence == rhs.sequence &&
			this->payload == rhs.payload;
	}
};     // struct basic_frame

typedef basic_frame<MSG> msg_frame;
typedef basic_frame<RPY> rpy_frame;
typedef basic_frame<ERR> err_frame;
typedef basic_frame<NUL> nul_frame;

struct ans_frame : public basic_frame<ANS> {
	ans_frame()
		: basic_frame<ANS>()
		, answer(0)
	{
	}

	unsigned int answer;

	bool
	operator==(const ans_frame &rhs) const
	{
		return this->answer == rhs.answer &&
			basic_frame<ANS>::operator==(rhs);
	}
};     // struct ans_frame

struct seq_frame {
	static const core_message_types header() { return SEQ; }

	unsigned int channel;
	unsigned int acknowledgement;
	unsigned int window;
};     // struct seq_frame

typedef boost::variant<
	msg_frame
	, rpy_frame
	, ans_frame
	, err_frame
	, nul_frame
	, seq_frame
	>
frame;
}      // namespace beep
#endif // BEEP_FRAME_HEAD
