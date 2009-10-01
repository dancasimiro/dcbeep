/// \file profile-stream.hpp
///
/// UNCLASSIFIED
#ifndef BEEP_PROFILE_STREAM_HEAD
#define BEEP_PROFILE_STREAM_HEAD 1
#include <istream>

namespace std {

inline
ostream&
operator<<(ostream &strm, const beep::profile &profile)
{
	if (strm) {
		strm << profile.uri();
	}
	return strm;
}

}	   // namespace std
#endif // BEEP_PROFILE_STREAM_HEAD
