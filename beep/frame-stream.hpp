/// \file frame-stream.hpp
///
/// UNCLASSIFIED
#ifndef BEEP_FRAME_STREAM_HEAD
#define BEEP_FRAME_STREAM_HEAD 1

#include <istream>
#include "frame.hpp"
#include "frame-parser.hpp"

namespace beep {

class frame_stream_visitor : public boost::static_visitor<> {
public:
	frame_stream_visitor(std::ostream &a_stream)
		: boost::static_visitor<>()
		, stream_(a_stream)
	{
	}

	void operator()(const msg_frame &frm) const
	{
		if (stream_ << "MSG ") {
			if (stream_basic_frame(frm)) {
				if (stream_frame_end(frm)) {
					return;
				}
			}
		}
		stream_.setstate(std::ios::failbit);
	}

	void operator()(const rpy_frame &frm) const
	{
		if (stream_ << "RPY ") {
			if (stream_basic_frame(frm)) {
				if (stream_frame_end(frm)) {
					return;
				}
			}
		}
		stream_.setstate(std::ios::failbit);
	}

	void operator()(const ans_frame &frm) const
	{
		if (stream_ << "ANS ") {
			if (stream_basic_frame(frm)) {
				if (stream_ << ' ' << frm.answer) {
					if (stream_frame_end(frm)) {
						return;
					}
				}
			}
		}
		stream_.setstate(std::ios::failbit);
	}

	void operator()(const nul_frame &frm) const
	{
		if (stream_ << "NUL ") {
			if (stream_basic_frame(frm)) {
				if (stream_frame_end(frm)) {
					return;
				}
			}
		}
		stream_.setstate(std::ios::failbit);
	}

	void operator()(const err_frame &frm) const
	{
		if (stream_ << "ERR ") {
			if (stream_basic_frame(frm)) {
				if (stream_frame_end(frm)) {
					return;
				}
			}
		}
		stream_.setstate(std::ios::failbit);
	}

	void operator()(const seq_frame &seq) const
	{
		if (!(stream_ << "SEQ " << seq.channel << ' ' << seq.acknowledgement << ' ' << seq.window << terminator())) {
			stream_.setstate(std::ios::failbit);
		}
	}
private:
	std::ostream &stream_;

	template <core_message_types CoreFrameType>
	std::ostream &stream_basic_frame(const basic_frame<CoreFrameType> &frm) const
	{
		return
			stream_ << frm.channel << ' '
					<< frm.message << ' '
					<< (frm.more ? '*' : '.') << ' '
					<< frm.sequence << ' '
					<< frm.payload.size()
			;
	}

	template <core_message_types CoreFrameType>
	std::ostream &stream_frame_end(const basic_frame<CoreFrameType> &frm) const
	{
		return stream_ << terminator() << frm.payload << sentinel();
	}
};

inline
std::ostream&
operator<<(std::ostream &stream, const beep::frame &aFrame)
{
	// I could change this to use boost::spirit::karma in the future...
	if (stream) {
		using boost::apply_visitor;
		apply_visitor(frame_stream_visitor(stream), aFrame);
	}
	return stream;
}

inline
std::istream&
operator>>(std::istream &stream, beep::frame &aFrame)
{
	if (stream) {
		const std::ios::fmtflags given_flags = stream.flags();
		stream.unsetf(std::ios::skipws);

		std::vector<frame> frames;
		std::string leftover;
		/// \todo read only a single frame
		parse_frames(stream, frames, leftover);
		/// \todo restore "leftover" to stream

		if (frames.empty()) {
			stream.setstate(std::ios::failbit);
		} else {
			aFrame = frames.front();
			stream.setf(given_flags);
		}
	}
	return stream;
}

}      // namespace beep
#endif // BEEP_FRAME_STREAM_HEAD
