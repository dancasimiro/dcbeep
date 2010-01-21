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
		content += first->get_payload();
	}
	istringstream strm(content);
	strm >> out;
	out.set_type(lead.get_type());
}

}      // namespace beep
#endif // BEEP_MESSAGE_GENERATOR_HEAD
