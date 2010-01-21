/// \file  frame.hpp
/// \brief BEEP frame
///
/// UNCLASSIFIED
#ifndef BEEP_FRAME_HEAD
#define BEEP_FRAME_HEAD 1

#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix.hpp>

namespace beep {

enum core_message_types {
	MSG = 0,
	RPY = 1,
	ANS = 2,
	ERR = 3,
	NUL = 4,
};

namespace qi = boost::spirit::qi;

struct continuation_symbols : qi::symbols<char, bool> {
	continuation_symbols()
	{
		add
			("*", true)
			(".", false)
			;
	}
};

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
};

class frame {
public:
	template <typename Iterator> friend struct frame_parser;
	typedef std::string string_type;

	static const string_type &sentinel()
	{
		static const string_type sent("END\r\n");
		return sent;
	}

	static const string_type &terminator()
	{
		static const string_type term("\r\n");
		return term;
	}

	static char separator()
	{
		static const char s = ' ';
		return s;
	}
public:
	frame()
		: header_(MSG)
		, payload_()
		, channel_(0)
		, message_(0)
		, sequence_(0)
		, answer_(0)
		, more_(false)
	{
	}

	const unsigned int get_type() const { return header_; }
	const unsigned int get_channel() const { return channel_; }
	const unsigned int get_message() const { return message_; }
	const bool get_more() const { return more_; }
	const unsigned int get_sequence() const { return sequence_; }
	const string_type &get_payload() const { return payload_; }
	const unsigned int get_answer() const { return answer_; }

	void set_type(const unsigned int h) { header_ = h; }
	void set_channel(const unsigned int c) { channel_ = c; }
	void set_message(const unsigned int m) { message_ = m; }
	void set_more(const bool m) { more_ = m; }
	void set_sequence(const unsigned int s) { sequence_ = s; }
	void set_payload(const string_type &p) { payload_ = p; }
	void set_payload_vector(const std::vector<char> &v) { payload_.assign(v.begin(), v.end()); }
	void set_answer(const unsigned int a) { answer_ = a; }
private:
	unsigned int header_;
	string_type  payload_;
	unsigned int channel_;
	unsigned int message_;
	unsigned int sequence_;
	unsigned int answer_;
	bool         more_;
private:
	static continuation_symbols &get_continuation_symbols()
	{
		static continuation_symbols symbols;
		return symbols;
	}

	static message_symbols &get_message_symbols()
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
	};
public:
	static unsigned int message_lookup(const string_type &sym)
	{
		using std::transform;
		const message_symbols &table = get_message_symbols();
		// convert the symbol to lowercase
		string_type mySymbol = sym;
		transform(mySymbol.begin(), mySymbol.end(), mySymbol.begin(), tolower);
		const unsigned int * const ptr = table.find(mySymbol);
		if (!ptr) {
			throw std::range_error("This message symbol is unknown.");
		}
		return *ptr;
	}

	static bool continuation_lookup(const string_type &sym)
	{
		using std::transform;
		const continuation_symbols &table = get_continuation_symbols();
		// convert the symbol to lowercase
		string_type mySymbol = sym;
		transform(mySymbol.begin(), mySymbol.end(), mySymbol.begin(), tolower);
		const bool * const ptr = table.find(mySymbol);
		if (!ptr) {
			throw std::range_error("This continuation symbol is unknown.");
		}
		return *ptr;
	}

	// I may be able to replace these reverse lookups with a karma generator
	static string_type reverse_message_lookup(const unsigned int t)
	{
		using std::transform;

		const message_symbols &table = get_message_symbols();
		symbol_table_reverse_lookup<unsigned int> lookup(t);
		table.for_each(lookup);
		string_type symbol = lookup.symbol();
		if (symbol.empty()) {
			throw std::range_error("The message symbol lookup failed.");
		}
		transform(symbol.begin(), symbol.end(), symbol.begin(), toupper);
		return symbol;
	}

	static string_type reverse_continuation_lookup(const bool c)
	{
		const continuation_symbols &table = get_continuation_symbols();
		symbol_table_reverse_lookup<bool> lookup(c);
		table.for_each(lookup);
		const string_type symbol = lookup.symbol();
		if (symbol.empty()) {
			throw std::range_error("The continuation symbol lookup failed.");
		}
		return symbol;
	}
};     // class frame

inline
bool operator==(const frame &lhs, const frame &rhs)
{
	return lhs.get_type() == rhs.get_type() &&
		lhs.get_channel() == rhs.get_channel() &&
		lhs.get_message() == rhs.get_message() &&
		lhs.get_more() == rhs.get_more() &&
		lhs.get_sequence() == rhs.get_sequence() &&
		lhs.get_answer() == rhs.get_answer() &&
		lhs.get_payload() == rhs.get_payload();
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
			(no_case[frame::get_message_symbols()][bind(&frame::set_type, _val, _1)] >> ' '
			 >> uint_[bind(&frame::set_channel, _val, _1), _pass = _1 < 2147483648u] >> ' '    // channel
			 >> uint_[bind(&frame::set_message, _val, _1), _pass = _1 < 2147483648u] >> ' '    // msgno
			 >> frame::get_continuation_symbols()[bind(&frame::set_more, _val, _1)] >> ' '     // more
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
#endif // BEEP_FRAME_HEAD
