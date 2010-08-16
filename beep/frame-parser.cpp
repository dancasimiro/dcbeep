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

struct continuation_symbols : qi::symbols<char, bool> {
	continuation_symbols()
	{
		add
			("*", true)
			(".", false)
			;
	}
};     // struct continuation_symbols

inline
continuation_symbols &get_continuation_symbols()
{
	static continuation_symbols symbols;
	return symbols;
}

// this object is copied by the symbol::for_each function.
// Therefore, the symbol_ object is lost. Use an internal pointer
// to keep the results available to the calling function.
template <typename T>
class symbol_table_reverse_lookup {
private:
	typedef std::string string_type;
	typedef boost::shared_ptr<string_type> ptr_type;
	T        key_;
	ptr_type symbol_;
public:
	typedef T key_type;
	symbol_table_reverse_lookup(const key_type k) : key_(k), symbol_(new string_type) { }

	symbol_table_reverse_lookup(const symbol_table_reverse_lookup &src)
		: key_(src.key_), symbol_(src.symbol_)
	{
	}

	symbol_table_reverse_lookup &operator=(const symbol_table_reverse_lookup &src)
	{
		if (this != &src) {
			this->key_ = src.key_;
			this->symbol_ = src.symbol_;
		}
		return *this;
	}

	void operator()(const string_type &s, const key_type k)
	{
		if (k == key_) {
			*symbol_ = s;
		}
	}

	const string_type &symbol() const { return *symbol_; }
};     // symbol_table_reverse_lookup

template <typename T>
bool continuation_lookup(const T &sym)
{
	using std::transform;
	const continuation_symbols &table = get_continuation_symbols();
	// convert the symbol to lowercase
	T mySymbol = sym;
	transform(mySymbol.begin(), mySymbol.end(), mySymbol.begin(), tolower);
	const bool * const ptr = table.find(mySymbol);
	if (!ptr) {
		throw std::range_error("This continuation symbol is unknown.");
	}
	return *ptr;
}

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
struct frame_parser : qi::grammar<Iterator, frame()> {

	frame_parser() : frame_parser::base_type(frame_rule)
	{
		using qi::uint_;
		using qi::_1;
		using qi::_2;
		using qi::_3;
		using qi::_4;
		using qi::lit;
		using qi::skip;
		using qi::no_skip;
		using qi::omit;
		using qi::space;
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
		channel %= uint_[_pass = _1 < 2147483648u];
		message_number %= uint_[_pass = _1 < 2147483648u];
		more %= get_continuation_symbols();
		sequence_number %= uint_[_pass = _1 <= 4294967295u];
		size %= uint_[_pass = _1 < 2147483648u];
		answer_number %= uint_[_pass = _1 <= 4294967295u];

		//common %= skip(space)[channel >> message_number >> more >> sequence_number >> size[_a = _1]];

		payload %= repeat(_r1)[byte_];

		msg %= no_skip["MSG"]
			> skip(space)[channel]
			> skip(space)[message_number]
			> skip(space)[more]
			> skip(space)[sequence_number]
			> omit[skip(space)[size[_a = _1]]]
			> no_skip[terminator_rule]
			> no_skip[payload(_a)]
			> no_skip[trailer]
			;

		rpy %= no_skip["RPY"]
			> skip(space)[channel]
			> skip(space)[message_number]
			> skip(space)[more]
			> skip(space)[sequence_number]
			> omit[skip(space)[size[_a = _1]]]
			> no_skip[terminator_rule]
			> no_skip[payload(_a)]
			> no_skip[trailer]
			;

		ans %= no_skip["ANS"]
			> skip(space)[channel]
			> skip(space)[message_number]
			> skip(space)[more]
			> skip(space)[sequence_number]
			> omit[skip(space)[size[_a = _1]]]
			> skip(space)[answer_number]
			> no_skip[terminator_rule]
			> no_skip[payload(_a)]
			> no_skip[trailer]
			;

		err %= no_skip["ERR"]
			> skip(space)[channel]
			> skip(space)[message_number]
			> skip(space)[more]
			> skip(space)[sequence_number]
			> omit[skip(space)[size[_a = _1]]]
			> no_skip[terminator_rule]
			> no_skip[payload(_a)]
			> no_skip[trailer]
			;

		nul %= no_skip["NUL"]
			> skip(space)[channel]
			> skip(space)[message_number]
			> skip(space)[more]
			> skip(space)[sequence_number]
			> omit[skip(space)[size[_a = _1]]]
			> no_skip[terminator_rule]
			> no_skip[payload(_a)]
			> no_skip[trailer]
			;

		data %= msg
			| rpy
			| ans
			| err
			| nul
			;

		seq %= no_skip["SEQ"]
			> skip(space)[channel]
			> skip(space)[sequence_number]
			> skip(space)[size]
			> no_skip[terminator_rule]
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

	qi::rule<Iterator> terminator_rule;

	// header components
	qi::rule<Iterator, std::size_t()> channel;
	qi::rule<Iterator, std::size_t()> message_number;
	qi::rule<Iterator, bool()> more;
	qi::rule<Iterator, std::size_t()> sequence_number;
	qi::rule<Iterator, std::size_t()> size;
	qi::rule<Iterator, std::size_t()> answer_number;
	//qi::rule<Iterator, basic_frame(), qi::locals<std::size_t> > common;

	// supported data header types
	// RFC 3080: The BEEP Core
	qi::rule<Iterator, msg_frame(), qi::locals<std::size_t> > msg;
	qi::rule<Iterator, rpy_frame(), qi::locals<std::size_t> > rpy;
	qi::rule<Iterator, ans_frame(), qi::locals<std::size_t> > ans;
	qi::rule<Iterator, err_frame(), qi::locals<std::size_t> > err;
	qi::rule<Iterator, nul_frame(), qi::locals<std::size_t> > nul;

	// supported mapping header types
	// RFC 3081: Mapping the BEEP Core onto TCP
	qi::rule<Iterator, seq_frame()> seq;

	// rules to select the current header
	qi::rule<Iterator, frame()> data;
	qi::rule<Iterator, frame()> mapping;

	qi::rule<Iterator, std::string(std::size_t)> payload;
	qi::rule<Iterator> trailer;

	qi::rule<Iterator, frame()> frame_rule;
};     // struct frame_grammar

void
parse_frames(std::istream &stream, std::vector<frame> &frames, std::string &left_over)
{
	frame_parser<boost::spirit::istream_iterator> grammar;
	frame current;
	boost::spirit::istream_iterator begin(stream);
	const boost::spirit::istream_iterator end;
	// begin is updated on each pass to the next valid position
	while (parse(begin, end, grammar, current)) {
		frames.push_back(current);
		// check if there is a frame trailer at the end of this buffer
		if (begin != end && std::string(begin, end).find(sentinel()) == std::string::npos) {
			// Need to read more data before this partial message can be parsed.
			break;
		}
	}
	if (begin != end) { // there is a partial message at the end of the streambuf
		left_over = std::string(begin, end);
	}
}

frame
parse_frame(const std::string &content)
{
	frame_parser<std::string::const_iterator> grammar;
	frame myFrame;
	std::string::const_iterator start = content.begin();
	const std::string::const_iterator end = content.end();
	if (!parse(start, end, grammar, myFrame) || start != end) {
		throw std::runtime_error("Incomplete parse!");
	}
	return myFrame;
}

}      // namespace beep
