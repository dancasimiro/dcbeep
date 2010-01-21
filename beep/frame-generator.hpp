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
	myFrame.set_type(msg.get_type());
	myFrame.set_channel(chan.get_number());
	myFrame.set_message(chan.get_message_number());
	myFrame.set_more(false);
	myFrame.set_sequence(chan.get_sequence_number());
	myFrame.set_payload(msg.get_payload());
	myFrame.set_answer(chan.get_answer_number());
	*out++ = myFrame;
	chan.update(msg);
	return 1;
}

}      // namespace beep
#endif // BEEP_FRAME_GENERATOR_HEAD
