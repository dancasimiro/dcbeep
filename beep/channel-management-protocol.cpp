/// \file  channel-management-protocol.cpp
/// \brief Implements parsers and generators for channel management
///
/// UNCLASSIFIED
#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "channel-management-protocol.hpp"
#include "frame.hpp"

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/karma.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/fusion/adapted/struct/adapt_struct.hpp>
#include <boost/fusion/include/adapt_struct.hpp>

BOOST_FUSION_ADAPT_STRUCT(beep::cmp::greeting_message,
						  (std::vector<std::string>, profile_uris)
						 )

namespace beep {
namespace cmp {

namespace qi = boost::spirit::qi;
namespace karma = boost::spirit::karma;
namespace ascii = boost::spirit::ascii;
namespace phoenix = boost::phoenix;

#define INPUT_SKIPPER_RULE ((qi::space | qi::eol))

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
				 > lexeme[+(char_ - char_(_a))][bind(&profile_element::uri, _val)]
				> omit[char_(_a)]
				)
				^
				(lit("encoding") > '='
				 > omit[char_("'\"")[_a = _1]]
				 > lexeme[+(char_ - char_(_a))][bind(&profile_element::encoding, _val)]
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
				 > uint_[_pass = _1 < 2147483648u, bind(&start_message::channel, _val)]
				 > omit[char_(_a)]
				 )
				^
				(lit("serverName") > '='
				 > char_("'\"")[_a = _1]
				 > lexeme[+(char_ - char_(_a))][bind(&start_message::server_name, _val)]
				 > omit[char_(_a)]
				 )
				)
			>> ">"
			>> +profile_tag
			>> "</start>"
			;

		ok_tag = eps[_val = ok_message()] >> "<ok" > "/>";

		xml %=
			greeting_tag
			| profile_tag
			| start_channel_tag
			| ok_tag
			;

		profile_uri.name("Profile Element");
		profile_tag.name("Profile Tag");
		empty_greeting_tag.name("Empty Greeting Tag");
		greeting_tag.name("Greeting Tag");
		start_channel_tag.name("Start Channel Tag");
		ok_tag.name("OK tag");

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
};     // input_protocol_grammar

template <typename OutputIterator>
struct output_greeting_grammar
	: karma::grammar<OutputIterator, greeting_message()>
{
	output_greeting_grammar()
		: output_greeting_grammar::base_type(xml)
	{
		using karma::string;
		using karma::uint_;
		using namespace karma::labels;

		profile_uri = "<profile uri=\"" << string << "\" />";

		empty_greeting = "<greeting />";
		
		server_greeting =
			"<greeting>" << +profile_uri << "</greeting>"
			;

		greeting_tag = server_greeting | empty_greeting;
#if 0
		profile_tag =
			string("<profile ")
			<< "uri=\""
			<< string[bind(&profile_element::uri, _val)]
			<< "\""
			<< "/>"
			;

		start_channel_tag =
			omit[string("<start")]
			/*
			<< "number=\""
			<< uint_[_pass = (_1 < 2147483648u && _1 > 0), bind(&start_message::channel, _val)]
			<< '\"'
			<< ' '
			<< "serverName=\""
			<< string[bind(&start_message::server_name, _val)]
			<< '\"'
			<< '>'
			<< +profile_tag[bind(&start_message::profiles, _val)]
			*/
			<< "</start>"
			;
#endif
		xml =
			greeting_tag
			//| start_channel_tag
			//| profile_tag
			;
	}
	karma::rule<OutputIterator, std::string()> profile_uri;
	karma::rule<OutputIterator, void()> empty_greeting;
	karma::rule<OutputIterator, greeting_message()> server_greeting;
	karma::rule<OutputIterator, greeting_message()> greeting_tag;
	karma::rule<OutputIterator, profile_element()> profile_tag;
	karma::rule<OutputIterator, start_message()> start_channel_tag;
	karma::rule<OutputIterator, greeting_message()> xml;
};     // output_protocol_grammar

namespace detail {
template <typename T>
std::string do_generate(const T &in)
{
	std::string generated;
	output_greeting_grammar<std::back_insert_iterator<std::string> > my_grammar;
	std::back_insert_iterator<std::string> sink(generated);
	const bool result = karma::generate(sink, my_grammar, in);
	if (!result) {
		throw std::runtime_error("bad generation!");
	}
	return generated;
}

class node_to_message_visitor : public boost::static_visitor<message> {
public:
	message operator()(const greeting_message &greeting) const
	{
		message msg;
		msg.set_mime(mime::beep_xml());
		msg.set_type(RPY);
		msg.set_content(do_generate(greeting));
		return msg;
	}

	message operator()(const start_message &start) const
	{
		message msg;
		msg.set_mime(mime::beep_xml());
		msg.set_type(MSG);
		//msg.set_content(do_generate(start));
		return msg;
	}

	message operator()(const close_message &close) const
	{
		message msg;
		msg.set_mime(mime::beep_xml());
		msg.set_type(MSG);
		//msg.set_content(do_generate(close));
		return msg;
	}

	message operator()(const ok_message &ok) const
	{
		message msg;
		msg.set_mime(mime::beep_xml());
		msg.set_type(RPY);
		//msg.set_content(do_generate(ok));
		return msg;
	}

	message operator()(const error_message &error) const
	{
		message msg;
		msg.set_mime(mime::beep_xml());
		msg.set_type(RPY);
		//msg.set_content(do_generate(error));
		return msg;
	}

	message operator()(const profile_element &profile) const
	{
		message msg;
		msg.set_mime(mime::beep_xml());
		msg.set_type(RPY);
		//msg.set_content(do_generate(profile));
		return msg;
	}
};     // node_to_message_visitor
}      // namespace detail

protocol_node parse(const message &my_message)
{
	using qi::phrase_parse;
	const std::string msg_content = my_message.get_content();
	std::string::const_iterator i = msg_content.begin();
	const std::string::const_iterator end = msg_content.end();
	input_protocol_grammar<std::string::const_iterator> my_grammar;
	protocol_node my_node;
	if (!phrase_parse(i, end, my_grammar, INPUT_SKIPPER_RULE, my_node) /*|| i != end*/) {
		std::ostringstream estrm;
		estrm << "could not parse channel management message: "
			  << msg_content;
		throw std::runtime_error(estrm.str());
	}
	return my_node;
}

message generate(const protocol_node &my_node)
{
	using boost::apply_visitor;
	return apply_visitor(detail::node_to_message_visitor(), my_node);
}

} // namespace cmp
} // namespace beep
