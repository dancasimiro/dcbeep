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

//BOOST_FUSION_ADAPT_STRUCT(beep::cmp::profile_element,
//						 (std::string, uri)
//						 )

BOOST_FUSION_ADAPT_STRUCT(beep::cmp::greeting_message,
						  (std::vector<std::string>, profile_uris)
						 )

namespace beep {
namespace cmp {

namespace qi = boost::spirit::qi;
namespace karma = boost::spirit::karma;
namespace ascii = boost::spirit::ascii;

template <typename Iterator>
struct input_protocol_grammar : qi::grammar<Iterator, protocol_node(), ascii::space_type>
{
	input_protocol_grammar()
		: input_protocol_grammar::base_type(xml)
	{
		using qi::lit;
		using qi::lexeme;
		using qi::omit;
		using ascii::char_;
		using ascii::space;
		using ascii::string;
		using namespace qi::labels;
		using qi::_1;

		start_tag %=
			'<'
			>> !lit('/')
			>> lexeme[_r1]
			>> '>'
			;

		end_tag =
			"</"
			>> string(_r1)
			>> '>'
			;

		profile_uri %=
			"<profile" >> +space >> "uri="
					   >> omit[char_("'\"")[_a = _1]]
					   >> lexeme[+char_]
					   >> char_(_a) >> *space >> "/>"
			;

		empty_greeting_tag = "<greeting />";

		greeting_tag %=
			omit[start_tag(std::string("greeting"))[_a = _1]]
			>> +profile_uri
			> end_tag(_a)
			| empty_greeting_tag
			;

		xml %=
			greeting_tag
			;
	}

	qi::rule<Iterator, protocol_node(), ascii::space_type> xml;
	qi::rule<Iterator, std::string(std::string), ascii::space_type> start_tag;
	qi::rule<Iterator, void(std::string), ascii::space_type> end_tag;
	qi::rule<Iterator, std::string(), qi::locals<char>, ascii::space_type> profile_uri;
	qi::rule<Iterator, greeting_message(), qi::locals<std::string>, ascii::space_type> greeting_tag;
	qi::rule<Iterator, void(), ascii::space_type> empty_greeting_tag;
};     // input_protocol_grammar

template <typename OutputIterator>
struct output_greeting_grammar
	: karma::grammar<OutputIterator, greeting_message()>
{
	output_greeting_grammar()
		: output_greeting_grammar::base_type(greeting)
	{
		using karma::string;

		//profile = "<profile uri=\"" << string << "\" />";
		profile_uri = "<profile uri=\"" << string << "\" />";

		empty_greeting = "<greeting />";
		
		server_greeting =
			"<greeting>" << +profile_uri << "</greeting>"
			;

		greeting = server_greeting | empty_greeting;
	}
	//karma::rule<OutputIterator, profile_element()> profile;
	karma::rule<OutputIterator, std::string()> profile_uri;
	karma::rule<OutputIterator, void()> empty_greeting;
	karma::rule<OutputIterator, greeting_message()> server_greeting;
	karma::rule<OutputIterator, greeting_message()> greeting;
};

namespace detail {
class node_to_message_visitor : public boost::static_visitor<message> {
public:
	message operator()(const greeting_message &greeting) const
	{
		using std::string;
		message msg;
		msg.set_mime(mime::beep_xml());
		msg.set_type(RPY);

		std::string generated;
		output_greeting_grammar<std::back_insert_iterator<std::string> > my_grammar;
		std::back_insert_iterator<std::string> sink(generated);
		const bool result = karma::generate(sink, my_grammar, greeting);
		if (!result) {
			throw std::runtime_error("bad generation!");
		}
		msg.set_content(generated);
		return msg;
	}

	message operator()(const start_message &) const
	{
		return message();
	}

	message operator()(const close_message &) const
	{
		return message();
	}

	message operator()(const ok_message &) const
	{
		return message();
	}

	message operator()(const error_message &) const
	{
		return message();
	}

	message operator()(const profile_element &) const
	{
		return message();
	}
};     // node_to_message_visitor
}      // namespace detail

protocol_node parse(const message &my_message)
{
	using boost::spirit::ascii::space;
	using qi::phrase_parse;
	const std::string msg_content = my_message.get_content();
	std::string::const_iterator i = msg_content.begin();
	std::string::const_iterator end = msg_content.end();
	input_protocol_grammar<std::string::const_iterator> my_grammar;
	protocol_node my_node;
	if (!phrase_parse(i, end, my_grammar, space, my_node) || i != end) {
		throw std::runtime_error("could not parse channel management message!");
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
