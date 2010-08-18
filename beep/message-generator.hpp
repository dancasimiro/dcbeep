/// \file message-generator.hpp
/// \brief Make BEEP messages from a series of frames
///
/// UNCLASSIFIED
#ifndef BEEP_MESSAGE_GENERATOR_HEAD
#define BEEP_MESSAGE_GENERATOR_HEAD 1

#include <functional>
#include <set>

#include "frame.hpp"

namespace beep {

class message;
class message_compiler : public std::unary_function<frame, message> {
public:
	message_compiler();
	/// \brief compile a frame into a message
	/// \return true if a complete message was output
	bool operator()(const frame&, message&);
private:
	std::set<message> pending_; // these are partial messages
};     // message_compiler

}      // namespace beep
#endif // BEEP_MESSAGE_GENERATOR_HEAD
