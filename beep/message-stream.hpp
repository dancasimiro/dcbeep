/// \file message-stream.hpp
///
/// UNCLASSIFIED
#ifndef BEEP_MESSAGE_STREAM_HEAD
#define BEEP_MESSAGE_STREAM_HEAD 1

#include <iosfwd>
namespace beep {
std::ostream &operator<<(std::ostream &strm, const mime &mm);
std::ostream &operator<<(std::ostream &strm, const message &msg);
}      // namespace beep
#endif // BEEP_MESSAGE_STREAM_HEAD
