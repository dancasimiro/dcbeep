/// \file  channel-management-protocol.hpp
/// \brief Implements parsers and generators for channel management
///
/// UNCLASSIFIED
#ifndef BEEP_CMP_HEAD
#define BEEP_CMP_HEAD 1
#include <vector>
#include <string>
#include <boost/spirit/include/qi.hpp>
#include <boost/variant.hpp>

namespace beep {
namespace cmp {

/// 2.3.1.1 The Greeting Message
///
/// When a BEEP session is established, each BEEP peer signifies its
/// availability by immediately sending a positive reply with a message number
/// of zero that contains a "greeting" element, e.g.,
///
/// L: <wait for incoming connection>
/// I: <open connection>
/// L: RPY 0 0 . 0 110
/// L: Content-Type: application/beep+xml
/// L:
/// L: <greeting>
/// L:	<profile uri=’http://iana.org/beep/TLS’ />
/// L: </greeting> L: END
/// I: RPY 0 0 . 0 52
/// I: Content-Type: application/beep+xml
/// I:
/// I: <greeting />
/// I: END
///
/// Note that this example implies that the BEEP peer in the initiating
/// role waits until the BEEP peer in the listening role sends its
/// greeting -- this is an artifact of the presentation; in fact, both
/// BEEP peers send their replies independently.
///
/// The "greeting" element has two optional attributes ("features" and
/// "localize") and zero or more "profile" elements, one for each profile
/// supported by the BEEP peer acting in a server role:
///
/// * the "features" attribute, if present, contains one or more feature
///   tokens, each indicating an optional feature of the channel
///   management profile supported by the BEEP peer;
///
/// * the "localize" attribute, if present, contains one or more language
///   tokens (defined in [9]), each identifying a desirable
///   language tag to be used by the remote BEEP peer when generating
///   textual diagnostics for the "close" and "error" elements (the
///   tokens are ordered from most to least desirable); and,
///
/// * each "profile" element contained within the "greeting" element
///   identifies a profile, and unlike the "profile" elements that occur
///   within the "start" element, the content of each "profile" element
///   may not contain an optional initialization message.
struct greeting_message {
	std::vector<std::string> profile_uris;
	std::vector<std::string> features;
	std::vector<std::string> localizations;
};     // struct greeting_message

struct profile_element {
	std::string uri;
	std::string encoding;
	message initialization;
};     // struct profile_element

/// 2.3.1.2 The Start Message
///
/// When a BEEP peer wants to create a channel, it sends a "start"
/// element on channel zero, e.g.,
///
///    C: MSG 0 1 . 52 120
///    C: Content-Type: application/beep+xml
///    C:
///    C: <start number=’1’>
///    C:	<profile uri=’http://iana.org/beep/SASL/OTP’ />
///    C: </start>
///    C: END
///
/// The "start" element has a "number" attribute, an optional
/// "serverName" attribute, and one or more "profile" elements:
///
/// * the "number" attribute indicates the channel number (in the range
///   1..2147483647) used to identify the channel in future messages;
///
/// * the "serverName" attribute, an arbitrary string, indicates the
///   desired server name for this BEEP session; and,
///
/// * each "profile" element contained with the "start" element has a
///   "uri" attribute, an optional "encoding" attribute, and arbitrary
///   character data as content:
///    - the "uri" attribute authoritatively identifies the profile;
///    - the "encoding" attribute, if present, specifies whether the
///      content of the "profile" element is represented as a base64-
///      encoded string; and,
///    - the content of the "profile" element, if present, must be no
///      longer than 4K octets in length and specifies an initialization
///      message given to the channel as soon as it is created.
///
/// To avoid conflict in assigning channel numbers when requesting the
/// creation of a channel, BEEP peers acting in the initiating role use
/// only positive integers that are odd-numbered; similarly, BEEP peers
/// acting in the listening role use only positive integers that are
/// even-numbered.
///
/// The "serverName" attribute for the first successful "start" element
/// received by a BEEP peer is meaningful for the duration of the BEEP
/// session. If present, the BEEP peer decides whether to operate as the
/// indicated "serverName"; if not, an "error" element is sent in a
/// negative reply.
///
/// When a BEEP peer receives a "start" element on channel zero, it
/// examines each of the proposed profiles, and decides whether to use
/// one of them to create the channel. If so, the appropriate "profile"
/// element is sent in a positive reply; otherwise, an "error" element is
/// sent in a negative reply.
///
/// When creating the channel, the value of the "serverName" attribute
/// from the first successful "start" element is consulted to provide
/// configuration information, e.g., the desired server-side certificate
/// when starting the TLS transport security profile (Section 3.1).
///
/// For example, a successful channel creation might look like this:
///
///     C: MSG 0 1 . 52 178
///     C: Content-Type: application/beep+xml
///     C:
///     C: <start number=’1’>
///     C:	<profile uri=’http://iana.org/beep/SASL/OTP’ />
///     C:	<profile uri=’http://iana.org/beep/SASL/ANONYMOUS’ />
///     C: </start>
///     C: END
///     S: RPY 0 1 . 221 87
///     S: Content-Type: application/beep+xml
///     S:
///     S: <profile uri=’http://iana.org/beep/SASL/OTP’ />
///     S: END
///
/// Similarly, an unsuccessful channel creation might look like this:
///
///     C: MSG 0 1 . 52 120
///     C: Content-Type: application/beep+xml
///     C:
///     C: <start number=’2’>
///     C:    <profile uri=’http://iana.org/beep/SASL/OTP’ />
///     C: </start>
///     C: END
///     S: ERR 0 1 . 221 127
///     S: Content-Type: application/beep+xml
///     S:
///     S: <error code='501'>number attribute
///     S: in &lt;start&gt; element must be odd-valued</error>
///     S: END
///
/// Finally, here's an example in which an initialization element is
/// exchanged during channel creation:
///
///     C: MSG 0 1 . 52 158
///     C:
///     C: Content-Type: application/beep+xml
///     C: <start number=’1’>
///     C:    <profile uri=’http://iana.org/beep/TLS’>
///     C:        <![CDATA[<ready />]]>
///     C:    </profile>
///     C: </start>
///     C: END
///     S: RPY 0 1 . 110 121
///     S: Content-Type: application/beep+xml
///     S:
///     S: <profile uri=’http://iana.org/beep/TLS’>
///     S:     <![CDATA[<proceed />]]>
///     S: </profile>
///     S: END
struct start_message {
	profile_element profile;
	std::string server_name;
	unsigned int channel;
};     // struct start_message

/// 2.3.1.3 The Close Message
///
/// When a BEEP peer wants to close a channel, it sends a "close" element
/// on channel zero, e.g.,
///
///    C: MSG 0 2 . 235 71
///    C: Content-Type: application/beep+xml
///    C:
///    C: <close number=’1’ code=’200’ />
///    C: END
///
/// The "close" element has a "number" attribute, a "code" attribute, an
/// optional "xml:lang" attribute, and an optional textual diagnostic as
/// its content:
///
///  * the "number" attribute indicates the channel number;
///  * the "code" attribute is a three-digit reply code meaningful to
///    programs (c.f., Section 8);
///  * the "xml:lang" attribute identifies the language that the
///    element’s content is written in (the value is suggested, but not
///    mandated, by the "localize" attribute of the "greeting" element
///    sent by the remote BEEP peer); and,
///  * the textual diagnostic (which may be multiline) is meaningful to
///    implementers, perhaps administrators, and possibly even users, but
///    never programs.
///
/// Note that if the textual diagnostic is present, then the "xml:lang"
/// attribute is absent only if the language indicated as the remote BEEP
/// peer’s first choice is used.
///
/// If the value of the "number" attribute is zero, then the BEEP peer
/// wants to release the BEEP session (c.f., Section 2.4) -- otherwise
/// the value of the "number" attribute refers to an existing channel,
/// and the remainder of this section applies.
///
/// A BEEP peer may send a "close" message for a channel whenever all
/// "MSG" messages it has sent on that channel have been acknowledged.
/// (Acknowledgement consists of the first frame of a reply being
/// received by the BEEP peer that sent the MSG "message.")
///
/// After sending the "close" message, that BEEP peer must not send any
/// more "MSG" messages on that channel beeing closed until the reply to
/// the "close" message has been received (either by an "error" message
/// rejecting the request to close the channel, or by an "ok" message
/// subsequently followed by the channel being successfully started).
struct close_message {
	std::string language;
	std::string diagnostic;
	unsigned int channel;
	unsigned int code;
};     // close_message

/// When a BEEP peer agrees to close a channel (or release the BEEP
/// session), it sends an "ok" element in a positive reply.
///
/// The "ok" element has no attributes and no content.
struct ok_message {
};     // ok_message

/// When a BEEP peer declines the creation of a channel, it sends an
/// "error" element in a negative reply, e.g.,
///
///      I: MSG 0 1 . 52 115
///      I: Content-Type: application/beep+xml
///      I:
///      I: <start number='2'>
///      I:    <profile uri='http://iana.org/beep/FOO' />
///      I: </start> 
///      I: END 
///      L: ERR 0 1 . 221 105 
///      L: Content-Type: application/beep+xml 
///      L: 
///      L: <error code='550'>all requested profiles are 
///      L: unsupported</error> 
///      L: END 
///
/// The "error" element has a "code" attribute, an optional "xml:lang"
/// attribute, and an optional textual diagnostic as its content:
///  *  the "code" attribute is a three-digit reply code meaningful to
///     programs (c.f., Section 8);
///  *  the "xml:lang" attribute identifies the language that the
///     element's content is written in (the value is suggested, but not
///     mandated, by the "localize" attribute of the "greeting" element
///     sent by the remote BEEP peer); and,
///  *  the textual diagnostic (which may be multiline) is meaningful to
///     implementers, perhaps administrators, and possibly even users, but
///     never programs.
///
/// \note that if the textual diagnostic is present, then the "xml:lang"
///       attribute is absent only if the language indicated as the remote BEEP
///       peer's first choice is used.
struct error_message {
	std::string language;
	std::string diagnostic;
	unsigned int code;
};     // error_message

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;

typedef boost::variant<
	greeting_message
	, start_message
	, ok_message
	, error_message
	, profile_element
	>
protocol_node;

template <typename Iterator>
struct protocol_grammar : qi::grammar<Iterator, protocol_node()>
{
	protocol_grammar()
		: protocol_grammar::base_type(xml)
	{
		using qi::lit;
		using qi::lexeme;
		using ascii::char_;
		using ascii::string;
		using namespace qi::labels;
		using qi::_1;

		start_tag %=
			'<'
			>> !lit('/')
			>> lexeme[+(char_ - '>')]
			>> '>'
			;

		end_tag =
			"</"
			>> string(_r1)
			>> '>'
			;

		xml %=
			start_tag[_a = _1]
			>> end_tag(_a)
			;
	}

	qi::rule<Iterator, protocol_node(), qi::locals<std::string>, ascii::space_type> xml;
	qi::rule<Iterator, std::string(), ascii::space_type> start_tag;
	qi::rule<Iterator, void(std::string), ascii::space_type> end_tag;
};     // protocol_grammar

inline protocol_node parse(const message &)
{
	return protocol_node();
}

inline message generate(const protocol_node &)
{
	return message();
}

}      // namespace cmp
}      // namespace beep
#endif // BEEP_CMP_HEAD
