/// \file message-stream.hpp
///
/// UNCLASSIFIED
#ifndef BEEP_MESSAGE_STREAM_HEAD
#define BEEP_MESSAGE_STREAM_HEAD 1

#include <istream>
#include <sstream>
#include <string>
#include <algorithm>

#include "message.hpp"

namespace std {

inline
istream&
operator>>(istream &strm, beep::mime &mm)
{
	if (strm) {
		/// \todo parse the encoding...
		string header, content_type;
		if (strm >> header >> content_type) {
			transform(header.begin(), header.end(), header.begin(), ::tolower);
			if (header == "content-type:") {
				mm.set_content_type(content_type);
			} else {
				strm.setstate(ios::badbit);
			}
		}
	}
	return strm;
}

inline
ostream&
operator<<(ostream &strm, const beep::mime &mm)
{
	if (strm) {
		if (mm == beep::mime::beep_xml()) {
			strm << "BEEP+XML MIME";
		} else {
			strm << "Unknown MIME (" << mm.get_content_type() << ")";
		}
	}
	return strm;
}

inline
istream&
operator>>(istream &strm, beep::message &msg)
{
	if (strm) {
		string line, mime_content;
		bool read_all_mime = false;
		bool missing_mime = false;
		// Try to read a MIME header from the input stream
		while (!read_all_mime && getline(strm, line)) {
			mime_content += line;
			if (line == "\r") {
				read_all_mime = true;
				istringstream lstrm(mime_content);
				beep::mime myMime;
				if (lstrm >> myMime) {
					msg.set_mime(myMime);
				} else {
					missing_mime = true;
				}
			}
		}
		// Try to recover if there was no MIME found.
		/// Try to set the message content to to the original value
		if ((!read_all_mime && strm.eof()) || missing_mime) {
			msg.set_content(mime_content);
			strm.setstate(ios::goodbit);
		} else if (strm) {
			/// Set the message content to the rest of the stream.
			vector<char> buffer(4096, '\0');
			streamsize total = 0;
			while (streamsize n = strm.readsome(&buffer[total], buffer.size() - total)) {
				total += n;
				if (static_cast<vector<char>::size_type>(total) == buffer.size()) {
					buffer.resize(buffer.size() * 2, '\0');
				}
			}
			buffer.resize(total);
			msg.set_content(string(buffer.begin(), buffer.end()));
		}
	}
	return strm;
}

inline
ostream&
operator<<(ostream &strm, const beep::message &msg)
{
	if (strm) {
		strm << "BEEP [" << msg.get_mime() << "] message";
	}
	return strm;
}

}      // namespace std
#endif // BEEP_MESSAGE_STREAM_HEAD
