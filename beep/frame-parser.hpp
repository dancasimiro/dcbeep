/// \file  frame-parser.hpp
/// \brief BEEP frame parser implemented with boost spirit 2.0
///
/// UNCLASSIFIED
#ifndef BEEP_FRAME_PARSER_HEAD
#define BEEP_FRAME_PARSER_HEAD 1

#include <string>
#include <stdexcept>
#include <algorithm>
#include <boost/shared_ptr.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix.hpp>

#include "frame.hpp"

namespace beep {
namespace qi = boost::spirit::qi;

struct continuation_symbols : qi::symbols<char, bool> {
	continuation_symbols()
	{
		add
			("*", true)
			(".", false)
			;
	}
};     // struct continuation_symbols

struct message_symbols : qi::symbols<char, unsigned int> {
	message_symbols()
	{
		add
			("msg", MSG)
			("rpy", RPY)
			("ans", ANS)
			("err", ERR)
			("nul", NUL)
			;
	}
};     // struct message_symbols

inline
continuation_symbols &get_continuation_symbols()
{
	static continuation_symbols symbols;
	return symbols;
}

inline
message_symbols &get_message_symbols()
{
	static message_symbols symbols;
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
unsigned int message_lookup(const T &sym)
{
	using std::transform;
	const message_symbols &table = get_message_symbols();
	// convert the symbol to lowercase
	T mySymbol = sym;
	transform(mySymbol.begin(), mySymbol.end(), mySymbol.begin(), tolower);
	const unsigned int * const ptr = table.find(mySymbol);
	if (!ptr) {
		throw std::range_error("This message symbol is unknown.");
	}
	return *ptr;
}

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

// I may be able to replace these reverse lookups with a karma generator
inline
std::string reverse_message_lookup(const unsigned int t)
{
	using std::transform;

	const message_symbols &table = get_message_symbols();
	symbol_table_reverse_lookup<unsigned int> lookup(t);
	table.for_each(lookup);
	std::string symbol = lookup.symbol();
	if (symbol.empty()) {
		throw std::range_error("The message symbol lookup failed.");
	}
	transform(symbol.begin(), symbol.end(), symbol.begin(), toupper);
	return symbol;
}

inline
std::string reverse_continuation_lookup(const bool c)
{
	const continuation_symbols &table = get_continuation_symbols();
	symbol_table_reverse_lookup<bool> lookup(c);
	table.for_each(lookup);
	const std::string symbol = lookup.symbol();
	if (symbol.empty()) {
		throw std::range_error("The continuation symbol lookup failed.");
	}
	return symbol;
}

template <typename Iterator>
struct frame_parser : qi::grammar<Iterator, frame(), qi::locals<std::size_t> > {

	frame_parser() : frame_parser::base_type(start)
	{
		using qi::eps;
		using qi::uint_;
		using qi::char_;
		using qi::repeat;
		using qi::raw;
		using qi::no_case;
		using qi::_1;
		using qi::_val;
		using qi::_a;
		using qi::_pass;

		using boost::phoenix::bind;

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

		// I am generous in what I accept, I don't enforce case in header type or END sentinel
		start = eps[_val = frame()] >> // initialize the frame object to the default
			// parse the header first
			(no_case[get_message_symbols()][bind(&frame::set_type, _val, _1)] >> ' '
			 >> uint_[bind(&frame::set_channel, _val, _1), _pass = _1 < 2147483648u] >> ' '    // channel
			 >> uint_[bind(&frame::set_message, _val, _1), _pass = _1 < 2147483648u] >> ' '    // msgno
			 >> get_continuation_symbols()[bind(&frame::set_more, _val, _1)] >> ' '     // more
			 >> uint_[bind(&frame::set_sequence, _val, _1), _pass = _1 <= 4294967295u] >> ' '  // seqno
			 >> uint_[_a = _1, _pass = _1 < 2147483648u]                                       // size
			 /// \todo Require ansno if header_symbol == "ANS"
			 >> -(' ' >> uint_[bind(&frame::set_answer, _val, _1), _pass = _1 <= 4294967295u]) // ansno
			 >> "\r\n"

			 // start the payload
			 //>> raw[repeat(_a)[char_]][bind(&frame::set_payload_i<Iterator>, _val, _1)]
			 >> repeat(_a)[char_][bind(&frame::set_payload_vector, _val, _1)]

			 // and the trailer
			 >> no_case["end\r\n"]
			 );
	}

	qi::rule<Iterator, frame(), qi::locals<std::size_t> > start;
};     // struct frame_grammar
}      // namespace beep
#endif // BEEP_FRAME_PARSER_HEAD
