/// \file  frame.hpp
/// \brief BEEP frame
///
/// UNCLASSIFIED
#ifndef BEEP_FRAME_HEAD
#define BEEP_FRAME_HEAD 1

#include <cstddef>
#include <string>
#include <stdexcept>
#include <boost/noncopyable.hpp>
#include <boost/throw_exception.hpp>
#include <boost/spirit/include/classic.hpp>
#include <boost/spirit/include/classic_grammar_def.hpp>
#include <boost/spirit/include/phoenix1.hpp>

namespace beep {

class frame {
public:
	typedef std::string string_type;
	typedef std::string header_type;

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

	static char separator()
	{
		static const char s = ' ';
		return s;
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

inline
bool operator==(const frame &lhs, const frame &rhs)
{
	return lhs.header() == rhs.header() &&
		lhs.channel() == rhs.channel() &&
		lhs.message() == rhs.message() &&
		lhs.more() == rhs.more() &&
		lhs.sequence() == rhs.sequence() &&
		lhs.answer() == rhs.answer() &&
		lhs.payload() == rhs.payload();
}

class frame_parsing_error : public std::runtime_error {
public:
	frame_parsing_error(const char *msg) : std::runtime_error(msg) {}
	frame_parsing_error(const std::string &msg) : std::runtime_error(msg) {}
	virtual ~frame_parsing_error() throw () {}
};

enum frame_syntax_errors {
	unknown_header_type,
	missing_space,
	missing_crlf,

	invalid_channel_number,
	invalid_message_number,
	invalid_more_character,
	invalid_sequence_number,
	invalid_size_number,
	invalid_answer_number,

	payload_size_mismatch,

	trailer_expected,
};

BOOST_SPIRIT_CLASSIC_NS::assertion<frame_syntax_errors> unknown_header(unknown_header_type);
BOOST_SPIRIT_CLASSIC_NS::assertion<frame_syntax_errors> expect_space(missing_space);
BOOST_SPIRIT_CLASSIC_NS::assertion<frame_syntax_errors> expect_crlf(missing_crlf);

BOOST_SPIRIT_CLASSIC_NS::assertion<frame_syntax_errors> expect_channel(invalid_channel_number);
BOOST_SPIRIT_CLASSIC_NS::assertion<frame_syntax_errors> expect_message(invalid_message_number);
BOOST_SPIRIT_CLASSIC_NS::assertion<frame_syntax_errors> expect_more(invalid_more_character);
BOOST_SPIRIT_CLASSIC_NS::assertion<frame_syntax_errors> expect_sequence(invalid_sequence_number);
BOOST_SPIRIT_CLASSIC_NS::assertion<frame_syntax_errors> expect_size(invalid_size_number);
BOOST_SPIRIT_CLASSIC_NS::assertion<frame_syntax_errors> expect_answer(invalid_answer_number);

BOOST_SPIRIT_CLASSIC_NS::assertion<frame_syntax_errors> expect_payload(payload_size_mismatch);
BOOST_SPIRIT_CLASSIC_NS::assertion<frame_syntax_errors> expect_trailer(trailer_expected);

// Set Frame Header Actor
struct set_frame_header_actor {
	template<typename T, typename ValueT>
	void act(T& ref, ValueT const & value) const
	{
		ref.set_header((*value));
	}

	template <typename T, typename IteratorT>
	void act(T& ref, IteratorT const& first, IteratorT const& last) const
	{
		typedef typename T::header_type header_type;
		header_type vt(first, last);
		ref.set_header(vt);
	}
};

template <typename T>
inline BOOST_SPIRIT_CLASSIC_NS::ref_value_actor<T, set_frame_header_actor>
set_frame_header_a(T &ref)
{
	return BOOST_SPIRIT_CLASSIC_NS::ref_value_actor<T, set_frame_header_actor>(ref);
}
// end header

// Set Frame Payload Actor
struct set_frame_payload_actor {
	template<typename T, typename ValueT>
	void act(T& ref, ValueT const & value) const
	{
		ref.set_payload((*value));
	}

	template <typename T, typename IteratorT>
	void act(T& ref, IteratorT const& first, IteratorT const& last) const
	{
		typedef typename T::string_type string_type;
		string_type vt(first, last);
		ref.set_payload(vt);
	}
};

template <typename T>
inline BOOST_SPIRIT_CLASSIC_NS::ref_value_actor<T, set_frame_payload_actor>
set_frame_payload_a(T &ref)
{
	return BOOST_SPIRIT_CLASSIC_NS::ref_value_actor<T, set_frame_payload_actor>(ref);
}
// end payload

// Set frame channel actor
struct set_frame_channel_actor {
	template<typename T, typename ValueT>
	void act(T& ref, ValueT const & value) const
	{
		ref.set_channel(value);
	}
};

template <typename T>
inline BOOST_SPIRIT_CLASSIC_NS::ref_value_actor<T, set_frame_channel_actor>
set_frame_channel_a(T &ref)
{
	return BOOST_SPIRIT_CLASSIC_NS::ref_value_actor<T, set_frame_channel_actor>(ref);
}
// end channel actor

struct set_frame_message_action {
	template<typename T, typename ValueT>
	void act(T& ref, ValueT const & value) const
	{
		ref.set_message(value);
	}
};

template <typename T>
inline BOOST_SPIRIT_CLASSIC_NS::ref_value_actor<T, set_frame_message_action>
set_frame_message_a(T &ref)
{
	return BOOST_SPIRIT_CLASSIC_NS::ref_value_actor<T, set_frame_message_action>(ref);
}

struct set_frame_continuation_actor {
	template<typename T, typename ValueT>
	void act(T& ref, ValueT const & value) const
	{
		ref.set_more(value == frame::intermediate());
	}
};

template <typename T>
inline BOOST_SPIRIT_CLASSIC_NS::ref_value_actor<T, set_frame_continuation_actor>
set_frame_continuation_a(T &ref)
{
	return BOOST_SPIRIT_CLASSIC_NS::ref_value_actor<T, set_frame_continuation_actor>(ref);
}

struct set_frame_sequence_actor {
	template<typename T, typename ValueT>
	void act(T& ref, ValueT const & value) const
	{
		ref.set_sequence(value);
	}
};

template <typename T>
inline BOOST_SPIRIT_CLASSIC_NS::ref_value_actor<T, set_frame_sequence_actor>
set_frame_sequence_a(T &ref)
{
	return BOOST_SPIRIT_CLASSIC_NS::ref_value_actor<T, set_frame_sequence_actor>(ref);
}

struct set_frame_answer_actor {
	template<typename T, typename ValueT>
	void act(T& ref, ValueT const & value) const
	{
		ref.set_answer(value);
	}
};

template <typename T>
inline BOOST_SPIRIT_CLASSIC_NS::ref_value_actor<T, set_frame_answer_actor>
set_frame_answer_a(T &ref)
{
	return BOOST_SPIRIT_CLASSIC_NS::ref_value_actor<T, set_frame_answer_actor>(ref);
}

struct frame_syntax : public BOOST_SPIRIT_CLASSIC_NS::grammar<frame_syntax> {
	template <typename ScannerT>
	struct definition {
		typedef BOOST_SPIRIT_CLASSIC_NS::rule<ScannerT>   rule_type;

		// standard BEEP header grammar
		rule_type sp;
		rule_type crlf;

		rule_type channel;
		rule_type msgno;
		rule_type more;
		rule_type seqno;
		rule_type size;
		rule_type ansno;

		rule_type common;
		rule_type message_frame;
		rule_type reply_frame;
		rule_type answer_frame;
		rule_type null_frame;
		rule_type error_frame;

		rule_type supported_frame;
		rule_type payload;
		rule_type trailer;
		rule_type top;

		definition(frame_syntax const& self)
		{
			using namespace BOOST_SPIRIT_CLASSIC_NS;
			using boost::ref;

			sp = expect_space(ch_p(' '));
			crlf = expect_crlf(str_p("\r\n"));

			// The channel number ("channel") must be a non-negative integer (in the 
			// range 0..2147483647).
			channel = expect_channel(limit_d(0u, 2147483647u)[uint_p[set_frame_channel_a(self.f)]]);

			// The message number ("msgno") must be a non-negative integer (in the 
			// range 0..2147483647) and have a different value than all other "MSG" 
			// messages on the same channel for which a reply has not been 
			// completely received. 
			msgno = expect_message(limit_d(0u, 2147483647u)[uint_p[set_frame_message_a(self.f)]]);

			// The continuation indicator ("more", one of: decimal code 42, "*", or 
			// decimal code 46, ".") specifies whether this is the final frame of 
			// the message: 
			//   intermediate ("*"): at least one other frame follows for the 
			//   message; or, 
			//  complete ("."): this frame completes the message. 
			more = expect_more(ch_p('*')[set_frame_continuation_a(self.f)] |
							   ch_p('.')[set_frame_continuation_a(self.f)]);

			// The sequence number ("seqno") must be a non-negative integer (in the 
			// range 0..4294967295) and specifies the sequence number of the first 
			// octet in the payload, for the associated channel (c.f., Section 
			// 2.2.1.2).
			seqno = expect_sequence(limit_d(0u, 4294967295u)[uint_p[set_frame_sequence_a(self.f)]]);

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
			size = expect_size(limit_d(0u, 2147483647u)[uint_p[assign_a(self.payload_size)]]);

			// The answer number ("ansno") must be a non-negative integer (in the 
			// range 0..4294967295) and must have a different value than all other 
			// answers in progress for the message being replied to. 
			ansno = expect_answer(limit_d(0u, 4294967295u)[uint_p[set_frame_answer_a(self.f)]]);

			common =
				channel >> sp >>
				msgno >> sp >>
				more >> sp >>
				seqno >> sp >>
				size
				;

			// The frame header consists of a three-character keyword (one of: 
			// "MSG", "RPY", "ERR", "ANS", or "NUL"), followed by zero or more 
			// parameters.  A single space character (decimal code 32, " ") 
			// separates each component.  The header is terminated with a CRLF pair.
			message_frame = str_p("MSG")[set_frame_header_a(self.f)] >> sp >> common;
			reply_frame = str_p("RPY")[set_frame_header_a(self.f)] >> sp >> common;
			answer_frame = str_p("ANS")[set_frame_header_a(self.f)] >> sp >> common >> sp >> ansno;
			null_frame = str_p("NUL")[set_frame_header_a(self.f)] >> sp >> common;
			error_frame = str_p("ERR")[set_frame_header_a(self.f)] >> sp >> common;
			supported_frame = (
							   message_frame
							   | reply_frame
							   | answer_frame
							   | null_frame
							   | error_frame
							   )
				>> crlf
				;

			payload = repeat_p(ref(self.payload_size))[anychar_p][set_frame_payload_a(self.f)];
			trailer = str_p("END\r\n");
			top = unknown_header(supported_frame) >> expect_payload(payload) >> expect_trailer(trailer);

            BOOST_SPIRIT_DEBUG_RULE(sp);
            BOOST_SPIRIT_DEBUG_RULE(crlf);
			BOOST_SPIRIT_DEBUG_RULE(channel);
			BOOST_SPIRIT_DEBUG_RULE(msgno);
			BOOST_SPIRIT_DEBUG_RULE(more);
			BOOST_SPIRIT_DEBUG_RULE(seqno);
			BOOST_SPIRIT_DEBUG_RULE(size);
			BOOST_SPIRIT_DEBUG_RULE(ansno);
            BOOST_SPIRIT_DEBUG_RULE(common);
			BOOST_SPIRIT_DEBUG_RULE(message_frame);
            BOOST_SPIRIT_DEBUG_RULE(reply_frame);
			BOOST_SPIRIT_DEBUG_RULE(answer_frame);
			BOOST_SPIRIT_DEBUG_RULE(null_frame);
			BOOST_SPIRIT_DEBUG_RULE(error_frame);
            BOOST_SPIRIT_DEBUG_RULE(supported_frame);
			BOOST_SPIRIT_DEBUG_RULE(payload);
			BOOST_SPIRIT_DEBUG_RULE(trailer);
            BOOST_SPIRIT_DEBUG_RULE(top);
		}

		const rule_type &start() const { return top; }
	}; // struct definition

	frame_syntax(frame &f_, std::size_t &s_) : f(f_), payload_size(s_) {}
	frame &f;
	std::size_t &payload_size;
};     // struct frame_syntax
}      // namespace beep
#endif // BEEP_FRAME_HEAD
