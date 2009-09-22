/// \file  frame.hpp
/// \brief BEEP frame
///
/// UNCLASSIFIED
#ifndef BEEP_FRAME_HEAD
#define BEEP_FRAME_HEAD 1

#include <cstddef>
#include <string>
#include <stdexcept>
#include <boost/spirit/include/classic.hpp>
#include <boost/spirit/include/classic_grammar_def.hpp>
#include <boost/spirit/include/phoenix1.hpp>

namespace beep {

class frame {
public:
	typedef char        byte_type;
	typedef std::string string_type;

	static const string_type &sentinel()
	{
		static const string_type sent("END\r\n");
		return sent;
	}

	static const string_type &msg()
	{
		static const string_type type("MSG");
		return type;
	}

	static const string_type &rpy()
	{
		static const string_type type("RPY");
		return type;
	}

	static const string_type &ans()
	{
		static const string_type type("ANS");
		return type;
	}

	static const string_type &err()
	{
		static const string_type type("ERR");
		return type;
	}

	static const string_type &nul()
	{
		static const string_type type("NUL");
		return type;
	}

	static char intermediate()
	{
		static const char i = '*';
		return i;
	}

	static char complete()
	{
		static const char c = '.';
		return c;
	}
public:
	frame()
		: header_()
		, payload_()
		, channel_(0)
		, message_(0)
		, sequence_(0)
		, answer_(0)
		, continuation_('\0')
	{
	}

	const string_type &header() const { return header_; }
	const unsigned int channel() const { return channel_; }
	const unsigned int message() const { return message_; }
	bool more() const { return continuation_ != complete(); }
	const unsigned int sequence() const { return sequence_; }
	const string_type &payload() const { return payload_; }
	const unsigned int answer() const { return answer_; }

	void set_header(const string_type &h) { header_ = h; }
	void set_channel(const unsigned int c) { channel_ = c; }
	void set_message(const unsigned int m) { message_ = m; }
	void set_more(const bool m) { continuation_ = m ? intermediate() : complete(); }
	void set_sequence(const unsigned int s) { sequence_ = s; }
	void set_payload(const string_type &p) { payload_ = p; }
	void set_answer(const unsigned int a) { answer_ = a; }
private:
	string_type  header_;
	string_type  payload_;
	unsigned int channel_;
	unsigned int message_;
	unsigned int sequence_;
	unsigned int answer_;
	char         continuation_;
};     // class frame

struct frame_closure : BOOST_SPIRIT_CLASSIC_NS::closure<frame_closure, frame> {
	member1 val;
};     // struct frame_closure

struct frame_syntax : public BOOST_SPIRIT_CLASSIC_NS::grammar<frame_syntax, frame_closure::context_t> {
	frame_syntax (std::size_t &s) : payload_size(s) {}
	template <typename ScannerT>
	struct definition {
		typedef BOOST_SPIRIT_CLASSIC_NS::rule<ScannerT, frame_closure::context_t> rule_type;
		typedef BOOST_SPIRIT_CLASSIC_NS::rule<ScannerT> top_level_rule_type;

		// standard BEEP header grammar
		rule_type channel;
		rule_type msgno;
		rule_type more;
		rule_type seqno;
		rule_type size;
		rule_type ansno;
		rule_type common;

		rule_type msg;
		rule_type rpy;
		rule_type ans;
		rule_type err;
		rule_type nul;

		rule_type header;
		rule_type payload;
		rule_type trailer;
		rule_type top;

		definition(const frame_syntax &self)
		{
			using namespace BOOST_SPIRIT_CLASSIC_NS;
			using namespace phoenix;
			using boost::ref;

			// The frame header consists of a three-character keyword (one of: 
			// "MSG", "RPY", "ERR", "ANS", or "NUL"), followed by zero or more 
			// parameters.  A single space character (decimal code 32, " ") 
			// separates each component.  The header is terminated with a CRLF pair.

			// The channel number ("channel") must be a non-negative integer (in the 
			// range 0..2147483647).
			channel = limit_d(0u, 2147483647u)[uint_p[bind(&frame::set_channel)(channel.val, arg1)]];

			// The message number ("msgno") must be a non-negative integer (in the 
			// range 0..2147483647) and have a different value than all other "MSG" 
			// messages on the same channel for which a reply has not been 
			// completely received. 
			msgno = limit_d(0u, 2147483647u)[uint_p[bind(&frame::set_message)(msgno.val, arg1)]];

			// The continuation indicator ("more", one of: decimal code 42, "*", or 
			// decimal code 46, ".") specifies whether this is the final frame of 
			// the message: 
			//   intermediate ("*"): at least one other frame follows for the 
			//   message; or, 
			//  complete ("."): this frame completes the message. 
			more = ch_p('*')[bind(&frame::set_more)(more.val, true)] |
				ch_p('.')[bind(&frame::set_more)(more.val, false)];

			// The sequence number ("seqno") must be a non-negative integer (in the 
			// range 0..4294967295) and specifies the sequence number of the first 
			// octet in the payload, for the associated channel (c.f., Section 
			// 2.2.1.2).
			seqno = limit_d(0u, 4294967295u)[uint_p[bind(&frame::set_sequence)(seqno.val, arg1)]];

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
			size = limit_d(0u, 2147483647u)[uint_p[assign_a(self.payload_size)]];

			// The answer number ("ansno") must be a non-negative integer (in the 
			// range 0..4294967295) and must have a different value than all other 
			// answers in progress for the message being replied to. 
			ansno = limit_d(0u, 4294967295u)[uint_p[bind(&frame::set_answer)(ansno.val, arg1)]];

			common =
				channel(common.val)[common.val = arg1] >> ch_p(' ') >>
				msgno(common.val)[common.val = arg1] >> ch_p(' ') >>
				more(common.val)[common.val = arg1] >> ch_p(' ') >>
				seqno(common.val)[common.val = arg1] >> ch_p(' ') >>
				size(common.val)[common.val = arg1];

			msg = str_p("MSG")[bind(&frame::set_header)(msg.val, construct_<std::string>(arg1, arg2))] >> ch_p(' ') >> common(msg.val)[msg.val = arg1] >> str_p("\r\n");
			rpy = str_p("RPY")[bind(&frame::set_header)(rpy.val, construct_<std::string>(arg1, arg2))] >> ch_p(' ') >> common(rpy.val)[rpy.val = arg1] >> str_p("\r\n");
			ans = str_p("ANS")[bind(&frame::set_header)(ans.val, construct_<std::string>(arg1, arg2))] >> ch_p(' ') >> common(ans.val)[ans.val = arg1] >> ch_p(' ') >> ansno(ans.val)[ans.val = arg1] >> str_p("\r\n");
			err = str_p("ERR")[bind(&frame::set_header)(err.val, construct_<std::string>(arg1, arg2))] >> ch_p(' ') >> common(err.val)[err.val = arg1] >> str_p("\r\n");
			nul = str_p("NUL")[bind(&frame::set_header)(nul.val, construct_<std::string>(arg1, arg2))] >> ch_p(' ') >> common(nul.val)[nul.val = arg1] >> str_p("\r\n");

			header =
				msg[self.val = arg1] |
				rpy[self.val = arg1] |
				ans[self.val = arg1] |
				err[self.val = arg1] |
				nul[self.val = arg1];
			payload = repeat_p(ref(self.payload_size))[anychar_p][bind(&frame::set_payload)(self.val, construct_<std::string>(arg1, arg2))];
			trailer = str_p("END\r\n");

			top = header >> payload >> trailer;
		}

		const rule_type &start() const { return top; }
	};

	std::size_t &payload_size;
};     // struct frame_syntax

class frame_parser {
public:
	frame_parser()
		: size_(0)
		, grammar_(size_)
	{
	}

	void operator()(const std::string &myInput, frame &f) const
	{
		using namespace BOOST_SPIRIT_CLASSIC_NS;
		using namespace phoenix;
		if (!parse(myInput.begin(), myInput.end(), grammar_[var(f) = arg1]).full) {
			// parsing failed
			throw std::runtime_error("fail");
		}
	}
private:
	std::size_t  size_; // memory to store the payload size
	frame_syntax grammar_;
};     // class frame_parser

}      // namespace beep
#endif // BEEP_FRAME_HEAD
