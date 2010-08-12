/// \file  frame-generator.hpp
/// \brief Make BEEP frames from a channel and message
///
/// UNCLASSIFIED

#ifndef BEEP_FRAME_GENERATOR_HEAD
#define BEEP_FRAME_GENERATOR_HEAD 1

#include "message.hpp"
#include "channel.hpp"
#include "frame.hpp"
#include "frame-parser.hpp" // for definition of beep::frame

namespace beep {

/// \return the message number used to create these frames.
template <typename OutputIterator>
void
make_frames(const message &msg, OutputIterator out)
{
	const channel ch = msg.get_channel();
	switch (msg.get_type()) {
	case MSG:
		{
			msg_frame MSG;
			MSG.channel = ch.get_number();
			MSG.message = ch.get_message_number();
			MSG.more = false;
			MSG.sequence = ch.get_sequence_number();
			MSG.payload = msg.get_payload();
			*out++ = MSG;
		}
		break;
	case RPY:
		{
			rpy_frame RPY;
			RPY.channel = ch.get_number();
			RPY.message = ch.get_message_number();
			RPY.more = false;
			RPY.sequence = ch.get_sequence_number();
			RPY.payload = msg.get_payload();
			*out++ = RPY;
		}
		break;
	case ANS:
		{
			ans_frame ANS;
			ANS.channel = ch.get_number();
			ANS.message = ch.get_message_number();
			ANS.more = false;
			ANS.sequence = ch.get_sequence_number();
			ANS.payload = msg.get_payload();
			ANS.answer = ch.get_answer_number();
			*out++ = ANS;
		}
		break;
	case ERR:
		{
			err_frame ERR;
			ERR.channel = ch.get_number();
			ERR.message = ch.get_message_number();
			ERR.more = false;
			ERR.sequence = ch.get_sequence_number();
			ERR.payload = msg.get_payload();
			*out++ = ERR;
		}
		break;
	case NUL:
		{
			nul_frame NUL;
			NUL.channel = ch.get_number();
			NUL.message = ch.get_message_number();
			NUL.more = false;
			NUL.sequence = ch.get_sequence_number();
			NUL.payload = msg.get_payload();
			*out++ = NUL;
		}
		break;
	case SEQ:
	default:
		std::cerr << "The message has an invalid frame type.\n";
		assert(false);
		/// \todo throw ?
	}
}

}      // namespace beep
#endif // BEEP_FRAME_GENERATOR_HEAD
