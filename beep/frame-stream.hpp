/// \file frame-stream.hpp
///
/// UNCLASSIFIED
#ifndef BEEP_FRAME_STREAM_HEAD
#define BEEP_FRAME_STREAM_HEAD 1

#include <iosfwd>
#include "frame.hpp"

namespace beep {

std::ostream &operator<<(std::ostream &stream, const frame &aFrame);
std::istream &operator>>(std::istream &stream, frame &aFrame);

}      // namespace beep
#endif // BEEP_FRAME_STREAM_HEAD
