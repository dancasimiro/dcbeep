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

namespace beep {
namespace cmp {

namespace karma = boost::spirit::karma;

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

message generate(const protocol_node &my_node)
{
	using boost::apply_visitor;
	return apply_visitor(detail::node_to_message_visitor(), my_node);
}

} // namespace cmp
} // namespace beep
