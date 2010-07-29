/// \file message-generator.hpp
/// \brief Make BEEP messages from a series of frames
///
/// UNCLASSIFIED
#ifndef BEEP_MESSAGE_GENERATOR_HEAD
#define BEEP_MESSAGE_GENERATOR_HEAD 1

#include <cassert>
#include <stdexcept>
#include <algorithm>
#include <string>
#include <sstream>
#include <boost/variant.hpp>

#include "message.hpp"
#include "message-stream.hpp"
namespace beep {

class frame_content_aggregator : public boost::static_visitor<> {
public:
	frame_content_aggregator(message::content_type &sink)
		: boost::static_visitor<>()
		, sink_(sink)
	{
	}

	/// MSG, RPY, ANS, NUL, and ERR frames
	template <core_message_types CoreFrameType>
	void operator()(const basic_frame<CoreFrameType> &frm) const
	{
		sink_ += frm.payload;
	}

	void operator()(const seq_frame &seq) const
	{
	}
private:
	message::content_type &sink_;
};

class frame_type_aggregator : public boost::static_visitor<> {
public:
	frame_type_aggregator(message &msg)
		: boost::static_visitor<>()
		, msg_(msg)
	{
	}

	template <typename T>
	void operator()(const T &frm) const
	{
		/// \todo add checking that all frames have the same values...
		msg_.set_type(frm.header());
		msg_.set_channel(frm.channel);
		msg_.set_number(frm.message);
	}

	void operator()(const seq_frame &frm) const
	{
		std::cerr << "cannot make messages from SEQ frames!\n";
		assert(false);
	}
private:
	message &msg_;
};

template <typename FwdIterator>
void
make_message(FwdIterator first, const FwdIterator last, message &out)
{
	using std::distance;
	using std::istringstream;
	using std::for_each;
	using boost::apply_visitor;

	if (!distance(first, last)) {
		throw std::runtime_error("Zero frames were provided to the message generator!");
	}
	frame_type_aggregator type_visitor(out);
	for_each(first, last, apply_visitor(type_visitor));

	message::content_type content;
	frame_content_aggregator visitor(content);
	for_each(first, last, apply_visitor(visitor));

	istringstream strm(content);
	strm >> out;
}

}      // namespace beep
#endif // BEEP_MESSAGE_GENERATOR_HEAD
