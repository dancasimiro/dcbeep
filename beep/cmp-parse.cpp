/// \file  cmp-parse.cpp
/// \brief Implements parsers and generators for channel management
///
/// UNCLASSIFIED
#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "channel-management-protocol.hpp"
#include "cmp-adapt.hpp"
#include "message.hpp"

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix.hpp>

namespace beep {
namespace cmp {

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
namespace phoenix = boost::phoenix;

#define INPUT_SKIPPER_RULE (qi::space)
typedef BOOST_TYPEOF(INPUT_SKIPPER_RULE) skipper_type;

template <typename Iterator>
struct input_protocol_grammar : qi::grammar<Iterator, protocol_node(), skipper_type>
{
	input_protocol_grammar()
		: input_protocol_grammar::base_type(xml)
	{
		using qi::lit;
		using qi::lexeme;
		using qi::raw;
		using qi::omit;
		using qi::uint_;
		using qi::on_error;
		using qi::eps;
		using qi::fail;
		using ascii::char_;
		using ascii::string;
		using namespace qi::labels;
		using qi::_1;

		using phoenix::construct;
		using phoenix::val;
		using phoenix::assign;
		using phoenix::push_back;

		// "profile_uri" is a profile element that is not allowed to contain
		// the encoding attribute, nor any content.
		profile_uri %=
			lit("<profile")
			>> (lit("uri") > '='
				> omit[char_("'\"")[_a = _1]]
				> lexeme[+(char_ - char_(_a))]
				> omit[char_(_a)]
				)
			> "/>"
			;

		profile_tag =
			"<profile"
			>> (
				(lit("uri") > '='
				 > omit[char_("'\"")[_a = _1]]
				 // "lexeme[+(char_ - char_(_a))]" outputs a vector<char>, so save contents by
				 // saying: _val.uri.assign(output.begin(), output.end())
				 > lexeme[+(char_ - char_(_a))][assign(bind(&profile_element::uri, _val), begin(_1), end(_1))]
				 > omit[char_(_a)]
				 )
				^
				(lit("encoding") > '='
				 > omit[char_("'\"")[_a = _1]]
				 > lexeme[+(char_ - char_(_a))][assign(bind(&profile_element::encoding, _val), begin(_1), end(_1))]
				 > omit[char_(_a)]
				 )
				)
			>> ("/>"
				| omit[raw[*char_]] >> "</profile>"
				)
			;

		empty_greeting_tag = "<greeting />";

		greeting_tag %=
			("<greeting>"
			 > +profile_uri
			 > "</greeting>")
			| empty_greeting_tag
			;

		start_channel_tag =
			"<start"
			>> (
				(lit("number") > '='
				 > char_("'\"")[_a = _1]
				 // meaning of "bind(&start_message::channel, _val) = _1" ----
				 //  This reads: Assign the output of the parser (uint_) to the "channel" member
				 //              variable of object _val. _val is an object of type start_message,
				 //              as defined in the rule definition.
				 > uint_[_pass = _1 < 2147483648u, bind(&start_message::channel, _val) = _1]
				 > omit[char_(_a)]
				 )
				^
				(lit("serverName") > '='
				 > char_("'\"")[_a = _1]
				 > lexeme[+(char_ - char_(_a))][assign(bind(&start_message::server_name, _val), begin(_1), end(_1))]
				 > omit[char_(_a)]
				 )
				)
			>> ">"
			>> +profile_tag[push_back(bind(&start_message::profiles, _val), _1)]
			>> "</start>"
			;

		ok_tag = eps[_val = ok_message()] >> "<ok" > "/>";

		close_channel_tag =
			eps[_val = close_message()]
			>> "<close"
			>> (
				(lit("number") > '='
				 > char_("'\"")[_a = _1]
				 > uint_[_pass = _1 < 2147483648u, bind(&close_message::channel, _val) = _1]
				 > omit[char_(_a)]
				 )
				^
				(lit("code") > '='
				 > char_("'\"")[_a = _1]
				 > uint_[bind(&close_message::code, _val) = _1]
				 > omit[char_(_a)]
				 )
				^
				(lit("xml:lang") > '='
				 > char_("'\"")[_a = _1]
				 > lexeme[+(char_ - char_(_a))][assign(bind(&close_message::language, _val), begin(_1), end(_1))]
				 > omit[char_(_a)]
				 )
				)
			> (
			   ('>'
				>> lexeme[*(char_ - '<')][assign(bind(&close_message::diagnostic, _val), begin(_1), end(_1))]
				>> "</close>")
			   | "/>"
			   )
			;

		error_tag =
			"<error"
			>> (
				(lit("code") > '='
				 > char_("'\"")[_a = _1]
				 > uint_[bind(&error_message::code, _val) = _1]
				 > omit[char_(_a)]
				 )
				^
				(lit("xml:lang") > '='
				 > char_("'\"")[_a = _1]
				 > lexeme[+(char_ - char_(_a))][assign(bind(&error_message::language, _val), begin(_1), end(_1))]
				 > omit[char_(_a)]
				 )
				)
			> (
				('>' >> lexeme[*(char_ - '<')][assign(bind(&error_message::diagnostic, _val), begin(_1), end(_1))]
				 >> "</error>")
				| "/>"
				)
			;

		xml %=
			greeting_tag
			| profile_tag
			| start_channel_tag
			| ok_tag
			| close_channel_tag
			| error_tag
			;

		profile_uri.name("Profile Element");
		profile_tag.name("Profile Tag");
		empty_greeting_tag.name("Empty Greeting Tag");
		greeting_tag.name("Greeting Tag");
		start_channel_tag.name("Start Channel Tag");
		ok_tag.name("OK tag");
		close_channel_tag.name("Close Channel Tag");
		error_tag.name("Error Tag");

		on_error<fail>
			(
			 xml
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

	qi::rule<Iterator, protocol_node(), skipper_type> xml;
	qi::rule<Iterator, std::string(), qi::locals<char>, skipper_type> profile_uri;
	qi::rule<Iterator, greeting_message(), skipper_type> greeting_tag;
	qi::rule<Iterator, void(), skipper_type> empty_greeting_tag;
	qi::rule<Iterator, profile_element(), qi::locals<char>, skipper_type> profile_tag;
	qi::rule<Iterator, start_message(), qi::locals<char>, skipper_type> start_channel_tag;
	qi::rule<Iterator, ok_message(), skipper_type> ok_tag;
	qi::rule<Iterator, close_message(), qi::locals<char>, skipper_type> close_channel_tag;
	qi::rule<Iterator, error_message(), qi::locals<char>, skipper_type> error_tag;
};     // input_protocol_grammar

protocol_node parse(const std::string &msg_content)
{
	using qi::phrase_parse;
	std::string::const_iterator i = msg_content.begin();
	const std::string::const_iterator end = msg_content.end();
	input_protocol_grammar<std::string::const_iterator> my_grammar;
	protocol_node my_node;
	if (!phrase_parse(i, end, my_grammar, INPUT_SKIPPER_RULE, my_node)) {
		std::ostringstream estrm;
		estrm << "could not parse channel management message: "
			  << msg_content;
		throw std::runtime_error(estrm.str());
	}
	return my_node;
}

protocol_node parse(const message &my_message)
{
	return parse(my_message.get_content());
}

} // namespace cmp
} // namespace beep
