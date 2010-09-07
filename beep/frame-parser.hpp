/// \file  frame-parser.hpp
/// \brief BEEP frame parser implemented with boost spirit 2.0
///
/// UNCLASSIFIED
#ifndef BEEP_FRAME_PARSER_HEAD
#define BEEP_FRAME_PARSER_HEAD 1

#include <iosfwd>
#include <string>
#include <vector>
#include "frame.hpp"

namespace beep {

std::string parse_frames(std::istream &stream, std::vector<frame> &frames);
frame parse_frame(std::istream &content);
frame parse_frame(const std::string &content);

}      // namespace beep
#endif // BEEP_FRAME_PARSER_HEAD
