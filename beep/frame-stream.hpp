/// \file frame-stream.hpp
///
/// UNCLASSIFIED
#ifndef BEEP_FRAME_STREAM_HEAD
#define BEEP_FRAME_STREAM_HEAD 1

#include <istream>
#include <sstream>
#include <string>
#include <vector>
#include "frame.hpp"

namespace std {

inline
ostream&
operator<<(ostream &stream, const beep::frame &aFrame)
{
	using namespace beep;
	if (stream) {
		const frame::string_type &payload = aFrame.get_payload();
		stream << frame::reverse_message_lookup(aFrame.get_type()) << frame::separator()
			   << aFrame.get_channel() << frame::separator()
			   << aFrame.get_message() << frame::separator()
			   << frame::reverse_continuation_lookup(aFrame.get_more()) << frame::separator()
			   << aFrame.get_sequence() << frame::separator()
			   << payload.size()
			;
		if (aFrame.get_type() == ANS) {
			stream << frame::separator() << aFrame.get_answer();
		}
		stream << frame::terminator();
		stream << payload << frame::sentinel();
	}
	return stream;
}

inline
istream&
operator>>(istream &stream, beep::frame &aFrame)
{
	using namespace beep;
	if (stream) {
		frame myFrame;
		string whole_header;
		unsigned int payload_size;
		if (getline(stream, whole_header)) {
			istringstream hstrm(whole_header);
			string header_type, cont;
			unsigned int ch, msg, seq;
			if (hstrm >> header_type >> ch >> msg >> cont >> seq >> payload_size) {
				myFrame.set_type(frame::message_lookup(header_type));
				myFrame.set_channel(ch);
				myFrame.set_message(msg);
				myFrame.set_more(frame::continuation_lookup(cont));
				myFrame.set_sequence(seq);
				if (myFrame.get_type() == ANS) {
					unsigned int ans;
					if (!(hstrm >> ans)) {
						stream.setstate(istream::badbit);
					} else {
						myFrame.set_answer(ans);
					}
				}
			} else {
				stream.setstate(istream::badbit);
			}
		}
		if (stream) {
			vector<char> buffer;
			buffer.resize(payload_size);
			if (stream.read(&buffer[0], payload_size)) {
				myFrame.set_payload(string(buffer.begin(), buffer.end()));
			}
		}
		if (stream) {
			string trailer;
			if (getline(stream, trailer) && trailer == "END\r") {
				swap(myFrame, aFrame);
			} else {
				stream.setstate(istream::badbit);
			}
		}
	}
	return stream;
}

}      // namespace std
#endif // BEEP_FRAME_STREAM_HEAD
