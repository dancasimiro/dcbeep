/// \file  cmp-generate.cpp
/// \brief Implements parsers and generators for channel management
///
/// UNCLASSIFIED
#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "channel-management-protocol.hpp"
#include "cmp-adapt.hpp"
#include "frame.hpp"

#include <boost/spirit/include/karma.hpp>
#include <boost/spirit/include/phoenix.hpp>

namespace beep {
namespace cmp {

namespace karma = boost::spirit::karma;
namespace phoenix = boost::phoenix;

template <typename OutputIterator>
struct output_greeting_grammar
	: karma::grammar<OutputIterator, greeting_message()>
{
	output_greeting_grammar()
		: output_greeting_grammar::base_type(greeting_tag)
	{
		using karma::string;

		profile_uri = "<profile uri=\"" << string << "\" />";

		empty_greeting = "<greeting />";
		
		server_greeting =
			"<greeting>" << +profile_uri << "</greeting>"
			;

		greeting_tag = server_greeting | empty_greeting;
	}
	karma::rule<OutputIterator, std::string()> profile_uri;
	karma::rule<OutputIterator, void()> empty_greeting;
	karma::rule<OutputIterator, greeting_message()> server_greeting;
	karma::rule<OutputIterator, greeting_message()> greeting_tag;
};     // output_greeting_grammar

template <typename OutputIterator>
struct output_start_grammar
	: karma::grammar<OutputIterator, start_message()>
{
	output_start_grammar()
		: output_start_grammar::base_type(start_channel_tag)
	{
		using karma::string;
		using karma::uint_;
		using namespace karma::labels;
		using karma::_1;

		profile_tag =
			string("<profile ")
			<< "uri=\""
			<< string[_1 = bind(&profile_element::uri, _val)]
			<< "\""
			<< " />"
			;

		start_channel_tag =
			string("<start ")
			<< "number=\""
			<< uint_[_1 = bind(&start_message::channel, _val), _pass = (_1 < 2147483648u && _1 > 0)]
			<< '\"'
			<< ' '
			<< "serverName=\""
			<< string[_1 = bind(&start_message::server_name, _val)]
			<< '\"'
			<< '>'
			<< (+profile_tag)[_1 = bind(&start_message::profiles, _val)]
			<< "</start>"
			;
	}
	karma::rule<OutputIterator, profile_element()> profile_tag;
	karma::rule<OutputIterator, start_message()> start_channel_tag;
};     // output_start_grammar

template <typename OutputIterator>
struct output_close_grammar
	: karma::grammar<OutputIterator, close_message()>
{
	output_close_grammar()
		: output_close_grammar::base_type(close_channel_tag)
	{
		using karma::uint_;
		using karma::string;
		using namespace karma::labels;
		using karma::_1;

		close_channel_tag =
			string("<close ")
			<< "number=\""
			<< uint_[_1 = bind(&close_message::channel, _val), _pass = (_1 < 2147483648u && _1 > 0)]
			<< '\"'
			<< ' '
			<< "code=\""
			<< uint_[_1 = bind(&close_message::code, _val)]
			<< '\"'
			<< " />"
			/// \todo add the language attribute
			/// \todo add the textual diagnostic content
			;
	}
	karma::rule<OutputIterator, close_message()> close_channel_tag;
	//karma::rule<OutputIterator, error_message()> error_tag;
};     // output_close_grammar

namespace detail {
template <class Generator, typename T>
std::string do_generate(const Generator &generator, const T &in)
{
	std::string generated;
	std::back_insert_iterator<std::string> sink(generated);
	if (!karma::generate(sink, generator, in)) {
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
		const output_greeting_grammar<std::back_insert_iterator<std::string> > grammar;
		msg.set_content(do_generate(grammar, greeting));
		return msg;
	}

	message operator()(const start_message &start) const
	{
		message msg;
		msg.set_mime(mime::beep_xml());
		msg.set_type(MSG);
		const output_start_grammar<std::back_insert_iterator<std::string> > grammar;
		msg.set_content(do_generate(grammar, start));
		return msg;
	}

	message operator()(const close_message &close) const
	{
		message msg;
		msg.set_mime(mime::beep_xml());
		msg.set_type(MSG);
		const output_close_grammar<std::back_insert_iterator<std::string> > grammar;
		msg.set_content(do_generate(grammar, close));
		return msg;
	}

	message operator()(const ok_message &) const
	{
		message msg;
		msg.set_mime(mime::beep_xml());
		msg.set_type(RPY);
		msg.set_content("<ok />");
		return msg;
	}

	message operator()(const error_message &/*error*/) const
	{
		message msg;
		msg.set_mime(mime::beep_xml());
		msg.set_type(RPY);
		//msg.set_content(do_generate(error));
		return msg;
	}

	message operator()(const profile_element &/*profile*/) const
	{
		message msg;
		msg.set_mime(mime::beep_xml());
		msg.set_type(RPY);
		//msg.set_content(do_generate(profile));
		return msg;
	}
};     // node_to_message_visitor

}      // namespace detail

message generate(const protocol_node &my_node)
{
	using boost::apply_visitor;
	return apply_visitor(detail::node_to_message_visitor(), my_node);
}

} // namespace cmp
} // namespace beep
