/// \file  frame-parser.cpp
/// \brief BEEP frame parser implemented with boost spirit 2.0
///
/// UNCLASSIFIED
#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "frame-parser.hpp"
#include <string>
#include <stdexcept>
#include <algorithm>

#include <boost/shared_ptr.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/fusion/adapted/struct/adapt_struct.hpp>
#include <boost/fusion/include/adapt_struct.hpp>

namespace beep {
namespace phoenix = boost::phoenix;
namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;

#define INPUT_SKIPPER_RULE (qi::space - terminator())
typedef BOOST_TYPEOF(INPUT_SKIPPER_RULE) skipper_type;

struct continuation_symbols_ : qi::symbols<char, bool> {
	continuation_symbols_()
	{
		add
			("*", true)
			(".", false)
			;
	}
} continuation_symbols;     // struct continuation_symbols

}      // namespace beep

BOOST_FUSION_ADAPT_STRUCT(beep::seq_frame,
						  (unsigned int, channel)
						  (unsigned int, acknowledgement)
						  (unsigned int, window)
						  )

BOOST_FUSION_ADAPT_STRUCT(beep::msg_frame,
						  (unsigned int, channel)
						  (unsigned int, message)
						  (bool, more)
						  (unsigned int, sequence)
						  (std::string, payload)
						  )

BOOST_FUSION_ADAPT_STRUCT(beep::rpy_frame,
						  (unsigned int, channel)
						  (unsigned int, message)
						  (bool, more)
						  (unsigned int, sequence)
						  (std::string, payload)
						  )

BOOST_FUSION_ADAPT_STRUCT(beep::ans_frame,
						  (unsigned int, channel)
						  (unsigned int, message)
						  (bool, more)
						  (unsigned int, sequence)
						  (unsigned int, answer)
						  (std::string, payload)
						  )

BOOST_FUSION_ADAPT_STRUCT(beep::err_frame,
						  (unsigned int, channel)
						  (unsigned int, message)
						  (bool, more)
						  (unsigned int, sequence)
						  (std::string, payload)
						  )

BOOST_FUSION_ADAPT_STRUCT(beep::nul_frame,
						  (unsigned int, channel)
						  (unsigned int, message)
						  (bool, more)
						  (unsigned int, sequence)
						  (std::string, payload)
						  )

namespace beep {

template <typename Iterator>
struct frame_parser : qi::grammar<Iterator, frame(), skipper_type> {

	frame_parser() : frame_parser::base_type(frame_rule)
	{
		using qi::uint_;
		using qi::_1;
		using qi::_2;
		using qi::_3;
		using qi::_4;
		using qi::no_skip;
		using qi::omit;
		using qi::repeat;
		using qi::on_error;
		using qi::fail;
		using qi::byte_;
		using namespace qi::labels;

		using phoenix::construct;
		using phoenix::val;
		// The channel number ("channel") must be a non-negative integer (in the
		// range 0..2147483647).

		// The message number ("msgno") must be a non-negative integer (in the
		// range 0..2147483647) and have a different value than all other "MSG"
		// messages on the same channel for which a reply has not been
		// completely received.

		// The continuation indicator ("more", one of: decimal code 42, "*", or 
		// decimal code 46, ".") specifies whether this is the final frame of 
		// the message: 
		//   intermediate ("*"): at least one other frame follows for the 
		//   message; or, 
		//  complete ("."): this frame completes the message. 

		// The sequence number ("seqno") must be a non-negative integer (in the 
		// range 0..4294967295) and specifies the sequence number of the first 
		// octet in the payload, for the associated channel (c.f., Section 
		// 2.2.1.2).

		// The payload size ("size") must be a non-negative integer (in the 
		// range 0..2147483647) and specifies the exact number of octets in the 
		// payload.  (This does not include either the header or trailer.) 
		// Note that a frame may have an empty payload, e.g., 
		//  S: RPY 0 1 * 287 20 
		//  S:     ... 
		//  S:     ... 
		//  S: END 
		//  S: RPY 0 1 . 307 0 
		//  S: END

		// The answer number ("ansno") must be a non-negative integer (in the 
		// range 0..4294967295) and must have a different value than all other 
		// answers in progress for the message being replied to.

		// The frame header consists of a three-character keyword (one of: 
		// "MSG", "RPY", "ERR", "ANS", or "NUL"), followed by zero or more 
		// parameters.  A single space character (decimal code 32, " ") 
		// separates each component.  The header is terminated with a CRLF pair.

		// This parser also knows about the "SEQ" frame header that is defined in RFC 3081 to
		// support the TCP/IP transport backing. In the future, I should only enable the SEQ
		// keyword if the TCP/IP transport is in use. However, it is currently always "on"
		// because the TCP/IP transport is the only one that I have implemented.
		terminator_rule = terminator();
		trailer = sentinel(); // terminator (\r\n) is included in sentinel().

		// header components
		channel = uint_[_pass = _1 < 2147483648u, _val = _1];
		message_number = uint_[_pass = _1 < 2147483648u, _val = _1];
		more %= continuation_symbols;
		sequence_number = uint_[_pass = _1 <= 4294967295u, _val = _1];
		size = uint_[_pass = _1 < 2147483648u, _val = _1];
		answer_number = uint_[_pass = _1 <= 4294967295u, _val = _1];

		//common %= skip(space)[channel >> message_number >> more >> sequence_number >> size[_a = _1]];

		payload %= no_skip[repeat(_r1)[byte_]];

		msg %= "MSG"
			> channel
			> message_number
			> more
			> sequence_number
			> omit[size[_a = _1]]
			> terminator_rule
			> payload(_a)
			> trailer
			;

		rpy %= "RPY"
			> channel
			> message_number
			> more
			> sequence_number
			> omit[size[_a = _1]]
			> terminator_rule
			> payload(_a)
			> trailer
			;

		ans %= "ANS"
			> channel
			> message_number
			> more
			> sequence_number
			> omit[size[_a = _1]]
			> answer_number
			> terminator_rule
			> payload(_a)
			> trailer
			;

		err %= "ERR"
			> channel
			> message_number
			> more
			> sequence_number
			> omit[size[_a = _1]]
			> terminator_rule
			> payload(_a)
			> trailer
			;

		nul %= "NUL"
			> channel
			> message_number
			> more
			> sequence_number
			> omit[size[_a = _1]]
			> terminator_rule
			> payload(_a)
			> trailer
			;

		data %= msg
			| rpy
			| ans
			| err
			| nul
			;

		seq %= "SEQ"
			> channel
			> sequence_number
			> size
			> terminator_rule
			;
		mapping %= seq;

		frame_rule %= data | mapping;

		terminator_rule.name("terminator");
		trailer.name("trailer");
		channel.name("channel");
		message_number.name("message_number");
		more.name("more");
		sequence_number.name("sequence_number");
		size.name("size");
		//common.name("common");
		payload.name("payload");
		msg.name("MSG");
		rpy.name("RPY");
		ans.name("ANS");
		err.name("ERR");
		nul.name("NUL");
		seq.name("SEQ");
		data.name("data");
		mapping.name("mapping");
		frame_rule.name("frame");

		on_error<fail>
			(
			 frame_rule
			 , std::cout
			 << val("Error! Expecting ")
			 << _4
			 << val(" here: \"")
			 << construct<std::string>(_3, _2)
			 << val("\"")
			 << val(" from: \"")
			 << construct<std::string>(_1, _3)
			 << std::endl
			 );
	}

	qi::rule<Iterator, skipper_type> terminator_rule;

	// header components
	qi::rule<Iterator, std::size_t(), skipper_type> channel;
	qi::rule<Iterator, std::size_t(), skipper_type> message_number;
	qi::rule<Iterator, bool(), skipper_type> more;
	qi::rule<Iterator, std::size_t(), skipper_type> sequence_number;
	qi::rule<Iterator, std::size_t(), skipper_type> size;
	qi::rule<Iterator, std::size_t(), skipper_type> answer_number;
	//qi::rule<Iterator, basic_frame(), qi::locals<std::size_t> > common;

	// supported data header types
	// RFC 3080: The BEEP Core
	qi::rule<Iterator, msg_frame(), qi::locals<std::size_t>, skipper_type> msg;
	qi::rule<Iterator, rpy_frame(), qi::locals<std::size_t>, skipper_type> rpy;
	qi::rule<Iterator, ans_frame(), qi::locals<std::size_t>, skipper_type> ans;
	qi::rule<Iterator, err_frame(), qi::locals<std::size_t>, skipper_type> err;
	qi::rule<Iterator, nul_frame(), qi::locals<std::size_t>, skipper_type> nul;

	// supported mapping header types
	// RFC 3081: Mapping the BEEP Core onto TCP
	qi::rule<Iterator, seq_frame(), skipper_type> seq;

	// rules to select the current header
	qi::rule<Iterator, frame(), skipper_type> data;
	qi::rule<Iterator, frame(), skipper_type> mapping;

	qi::rule<Iterator, std::string(std::size_t), skipper_type> payload;
	qi::rule<Iterator, skipper_type> trailer;

	qi::rule<Iterator, frame(), skipper_type> frame_rule;
};     // struct frame_grammar

void
parse_frames(std::istream &stream, std::vector<frame> &frames)
{
	const std::ios::fmtflags given_flags = stream.flags();
	stream.unsetf(std::ios::skipws);
	const frame_parser<boost::spirit::istream_iterator> grammar;
	frame current;
	boost::spirit::istream_iterator begin(stream);
	const boost::spirit::istream_iterator end;
	// begin is updated on each pass to the next valid position
	while (phrase_parse(begin, end, grammar, INPUT_SKIPPER_RULE, current)) {
		frames.push_back(current);
	}
	stream.setf(given_flags);
}

frame
parse_frame(const std::string &content)
{
	const frame_parser<std::string::const_iterator> grammar;
	frame myFrame;
	std::string::const_iterator start = content.begin();
	const std::string::const_iterator end = content.end();
	if (!phrase_parse(start, end, grammar, INPUT_SKIPPER_RULE, myFrame) || start != end) {
		throw std::runtime_error("Incomplete parse!");
	}
	return myFrame;
}

frame
parse_frame(std::istream &stream)
{
	const std::ios::fmtflags given_flags = stream.flags();
	stream.unsetf(std::ios::skipws);
	const frame_parser<boost::spirit::istream_iterator> grammar;
	frame myFrame;
	boost::spirit::istream_iterator begin(stream);
	const boost::spirit::istream_iterator end;
	if (!phrase_parse(begin, end, grammar, INPUT_SKIPPER_RULE, myFrame)) {
		throw std::runtime_error("Bad frame (stream) parse!");
	}
	// the parser consumes one extra character, replace it here
	stream.unget();
	stream.setf(given_flags);
	return myFrame;
}

}      // namespace beep
