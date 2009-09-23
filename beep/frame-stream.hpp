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

ostream&
operator<<(ostream &stream, const beep::frame &aFrame)
{
	using beep::frame;
	if (stream) {
		const frame::header_type header = aFrame.header();
		const frame::string_type payload = aFrame.payload();
		const char more =
			aFrame.more() ? frame::intermediate() : frame::complete();
		stream << header << frame::separator()
			   << aFrame.channel() << frame::separator()
			   << aFrame.message() << frame::separator()
			   << more << frame::separator()
			   << aFrame.sequence() << frame::separator()
			   << payload.size();
		if (header == frame::ans()) {
			stream << frame::separator() << aFrame.answer();
		}
		stream << frame::terminator();
		stream << payload << frame::sentinel();
	}
	return stream;
}

istream&
operator>>(istream &stream, beep::frame &aFrame)
{
	using beep::frame;
	if (stream) {
		frame myFrame;
		string whole_header;
		unsigned int payload_size;
		if (getline(stream, whole_header)) {
			istringstream hstrm(whole_header);
			string header_type;
			unsigned int ch, msg, seq;
			char cont;
			if (hstrm >> header_type >> ch >> msg >> cont >> seq >> payload_size) {
				myFrame.set_header(header_type);
				myFrame.set_channel(ch);
				myFrame.set_message(msg);
				myFrame.set_more(cont == frame::intermediate());
				myFrame.set_sequence(seq);
				if (header_type == frame::ans()) {
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
