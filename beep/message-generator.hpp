/// \file message-generator.hpp
/// \brief Make BEEP messages from a series of frames
///
/// UNCLASSIFIED
#ifndef BEEP_MESSAGE_GENERATOR_HEAD
#define BEEP_MESSAGE_GENERATOR_HEAD 1

#include <stdexcept>
#include <algorithm>
#include <string>
#include <sstream>

#include "message.hpp"
#include "message-stream.hpp"
namespace beep {

template <typename FwdIterator>
void
make_message(FwdIterator first, const FwdIterator last, message &out)
{
	using std::distance;
	using std::getline;
	using std::istringstream;
	using std::string;
	using std::transform;

	if (!distance(first, last)) {
		throw std::runtime_error("Zero frames were provided to the message generator!");
	}

	const frame lead = *first;
	message::content_type content;
	for (; first != last; ++first) {
		content += first->payload();
	}
	istringstream strm(content);
	strm >> out;

	if (lead.header() == frame::msg()) {
		out.set_type(message::MSG);
	} else if (lead.header() == frame::rpy()) {
		out.set_type(message::RPY);
	} else if (lead.header() == frame::ans()) {
		out.set_type(message::ANS);
	} else if (lead.header() == frame::err()) {
		out.set_type(message::ERR);
	} else if (lead.header() == frame::nul()) {
		out.set_type(message::NUL);
	} else {
		throw std::runtime_error("Unsupported frame header in message generator!");
	}
}

}      // namespace beep
#endif // BEEP_MESSAGE_GENERATOR_HEAD
