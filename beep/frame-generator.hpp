/// \file  frame-generator.hpp
/// \brief Make BEEP frames from a channel and message
///
/// UNCLASSIFIED

#ifndef BEEP_FRAME_GENERATOR_HEAD
#define BEEP_FRAME_GENERATOR_HEAD 1

#include <stdexcept>

#include "message.hpp"
#include "channel.hpp"
#include "frame.hpp"

namespace beep {

template <typename OutputIterator>
int
make_frames(const message &msg, channel &chan, OutputIterator out)
{
	frame myFrame;
	switch (msg.get_type()) {
	case message::MSG:
		myFrame.set_header(frame::msg());
		break;
	case message::RPY:
		myFrame.set_header(frame::rpy());
		break;
	case message::ANS:
		myFrame.set_header(frame::ans());
		break;
	case message::ERR:
		myFrame.set_header(frame::err());
		break;
	case message::NUL:
		myFrame.set_header(frame::nul());
		break;
	default:
		throw std::runtime_error("Unsupported frame header!");
	}
	myFrame.set_channel(chan.get_number());
	myFrame.set_message(chan.get_message_number());
	myFrame.set_more(false);
	myFrame.set_sequence(chan.get_sequence_number());
	myFrame.set_payload(msg.payload());
	myFrame.set_answer(chan.get_answer_number());
	*out++ = myFrame;
	chan.update(msg);
	return 1;
}

}      // namespace beep
#endif // BEEP_FRAME_GENERATOR_HEAD
