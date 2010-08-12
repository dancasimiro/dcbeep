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

#include <boost/variant.hpp>

namespace beep {

class message_to_frame_visitor : public boost::static_visitor<unsigned int> {
public:
	message_to_frame_visitor(const message &a_message, const channel &a_channel)
		: boost::static_visitor<unsigned int>()
		, msg_(a_message)
		, chan_(a_channel)
	{
	}

	/// MSG, RPY, NUL, and ERR frames
	template <core_message_types CoreFrameType>
	unsigned int operator()(basic_frame<CoreFrameType> &frm) const
	{
		frm.channel = chan_.get_number();
		frm.message = chan_.get_message_number();
		frm.more = false;
		frm.sequence = chan_.get_sequence_number();
		frm.payload = msg_.get_payload();
		return frm.message;
	}

	unsigned int operator()(ans_frame &ans) const
	{
		ans.channel = chan_.get_number();
		ans.message = chan_.get_message_number();
		ans.more = false;
		ans.sequence = chan_.get_sequence_number();
		ans.payload = msg_.get_payload();
		ans.answer = chan_.get_answer_number();
		return ans.message;
	}

	unsigned int operator()(seq_frame &/*seq*/) const
	{
		std::cerr << "warning: SEQ frames don't support message numbers.\n";
		return 0;
	}
private:
	const message &msg_;
	const channel &chan_;
};

/// \return the message number used to create these frames.
template <typename OutputIterator>
unsigned int
make_frames(const message &msg, channel &chan, OutputIterator out)
{
	using boost::apply_visitor;
	frame myFrame;
	switch (msg.get_type()) {
	case MSG:
		myFrame = msg_frame();
		break;
	case RPY:
		myFrame = rpy_frame();
		break;
	case ANS:
		myFrame = ans_frame();
		break;
	case ERR:
		myFrame = err_frame();
		break;
	case NUL:
		myFrame = nul_frame();
		break;
	case SEQ:
	default:
		std::cerr << "The message has an invalid frame type.\n";
		assert(false);
		/// \todo throw ?
	}
	const unsigned int message_number =
		apply_visitor(message_to_frame_visitor(msg, chan), myFrame);
	*out++ = myFrame;
	chan.update(msg);
	return message_number;
}

}      // namespace beep
#endif // BEEP_FRAME_GENERATOR_HEAD
