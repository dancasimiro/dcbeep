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
			strm << "Unknown MIME (" << mm.content_type() << ")";
		}
	}
	return strm;
}

inline
istream&
operator>>(istream &strm, beep::message &msg)
{
	if (strm) {
		string line;
		string final_content;
		while (getline(strm, line)) {
			istringstream lstrm(line);
			beep::mime myMime;
			if (lstrm >> myMime) {
				msg.set_mime(myMime);
			} else {
				final_content += line;
			}
		}
		msg.set_content(final_content);
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
