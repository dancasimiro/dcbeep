/// \file  channel-manager.hpp
/// \brief Add and remove channels from a session
///
/// UNCLASSIFIED
#ifndef BEEP_CHANNEL_MANAGER_HEAD
#define BEEP_CHANNEL_MANAGER_HEAD 1

#include <cassert>
#include <stdexcept>
#include <istream>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <algorithm>
#include <iterator>
#include <utility>
#include <functional>

#include "role.hpp"
#include "message.hpp"
#include "channel.hpp"
#include "profile.hpp"
#include "profile-stream.hpp"
#include "reply-code.hpp"

#include "tinyxml.h"

namespace beep {
/// Channel Management Protocol
namespace cmp {
namespace detail {

template <typename T>
struct profile_uri_extractor : std::unary_function<std::string, T> {
const std::string &operator()(const T &in) const
{
	return in.uri();
}
};

template <>
struct profile_uri_extractor<std::pair<const std::string, profile> >
	: std::unary_function<std::string, std::pair<const std::string, profile> > {
const std::string &operator()(const std::pair<const std::string, profile> &in) const
{
	return in.first;
}
};

profile uri_to_profile(const std::string &uri)
{
	profile p;
	p.set_uri(uri);
	return p;
}

bool message_has_element_named(const message &msg, const std::string &name)
{
	bool is = false;
	using std::getline;
	using std::string;
	using std::istringstream;
	using std::transform;

	istringstream strm(msg.content());
	string line;
	while (!is && getline(strm, line)) {
		transform(line.begin(), line.end(), line.begin(), ::tolower);
		is = (line.find(name) != string::npos);
	}
	return is;
}

}      // namespace detail

bool is_greeting_message(const message &msg)
{
	return detail::message_has_element_named(msg, "greeting");
}

bool is_start_message(const message &msg)
{
	return detail::message_has_element_named(msg, "start");
}

bool is_close_message(const message &msg)
{
	return detail::message_has_element_named(msg, "close");
}

bool is_ok_message(const message &msg)
{
	return detail::message_has_element_named(msg, "ok");
}

bool is_error_message(const message &msg)
{
	return detail::message_has_element_named(msg, "error");
}

class greeting {
public:
	typedef std::vector<std::string> uri_container_type;
	typedef uri_container_type::const_iterator uri_const_iterator;
	greeting()
		: uris_()
	{
	}

	// Accept forward iterators of profile objects
	template <typename FwdIterator>
	greeting(const FwdIterator first, const FwdIterator last)
		: uris_()
	{
		using std::distance;
		using std::transform;
		using std::iterator_traits;

		typedef typename iterator_traits<FwdIterator>::value_type value_type;
		uris_.resize(distance(first, last));
		transform(first, last, uris_.begin(),
				  detail::profile_uri_extractor<value_type>());
	}

	uri_const_iterator begin() const { return uris_.begin(); }
	uri_const_iterator end() const { return uris_.end(); }

	void push_back_uri(const std::string &uri) { uris_.push_back(uri); }
private:
	uri_container_type uris_;
};

class start {
public:
	typedef std::vector<profile>              profile_container;
	typedef profile_container::const_iterator profile_const_iterator;

	start()
		: channel_(0)
		, server_()
		, profiles_()
	{
	}

	unsigned int get_number() const { return channel_; }
	void set_number(const unsigned int c) { channel_ = c; }

	const std::string &get_server_name() const { return server_; }
	void set_server_name(const std::string &n) { server_ = n; }

	profile_const_iterator profiles_begin() const { return profiles_.begin(); }
	profile_const_iterator profiles_end() const { return profiles_.end(); }

	void push_back_profile(const profile &p) { profiles_.push_back(p); }
private:
	unsigned int      channel_; // number
	std::string       server_; // serverName
	profile_container profiles_;
};     // class start

/// The "close" element has a "number" attribute, a "code" attribute, an
/// optional "xml:lang" attribute, and an optional textual diagnostic as
/// its content:
///  *  the "number" attribute indicates the channel number;
///  *  the "code" attribute is a three-digit reply code meaningful to
///     programs (c.f., Section 8);
///  *  the "xml:lang" attribute identifies the language that the
///     element's content is written in (the value is suggested, but not
///     mandated, by the "localize" attribute of the "greeting" element
///     sent by the remote BEEP peer); and,
///  *  the textual diagnostic (which may be multiline) is meaningful to
///     implementers, perhaps administrators, and possibly even users, but
///     never programs.
class close {
public:
	close()
		: channel_(0)
		, code_(0)
	{
	}

	unsigned int number() const { return channel_; }
	unsigned int code() const { return code_; }

	void set_number(const unsigned int c) { channel_ = c; }
	void set_code(const unsigned int c) { code_ = c; }
private:
	unsigned int channel_;
	unsigned int code_;
};     // class close

/// When a BEEP peer agrees to close a channel (or release the BEEP
/// session), it sends an "ok" element in a positive reply.
///
/// The "ok" element has no attributes and no content.
class ok {
};

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
class error {
public:
	error()
		: code_(reply_code::success)
		, descr_("There is no error.")
	{
	}

	error(const reply_code::reply_code_type rc, const std::string &descr)
		: code_(rc)
		, descr_(descr)
	{
	}

	reply_code::reply_code_type code() const { return code_; }
	const std::string &description() const { return descr_; }

	void set_code(const reply_code::reply_code_type rc) { code_ = rc; }
	void set_description(const std::string &d) { descr_ = d; }
private:
	reply_code::reply_code_type code_;
	std::string                 descr_;
};

class XmlGreetingVisitor : public TiXmlVisitor {
public:
	XmlGreetingVisitor(greeting &g) : greeting_(g) {}
	virtual ~XmlGreetingVisitor() {}

	virtual bool VisitEnter(const TiXmlDocument &doc)
	{
		if (const TiXmlElement *element = doc.RootElement()) {
			return ("greeting" == element->ValueStr());
		}
		return false;
	}

	virtual bool VisitEnter(const TiXmlElement &element, const TiXmlAttribute *attribute)
	{
		if ("profile" == element.ValueStr()) {
			for (; attribute; attribute = attribute->Next()) {
				if (std::string("uri") == attribute->Name()) {
					greeting_.push_back_uri(attribute->ValueStr());
				}
			}
		}
		return true;
	}
private:
	greeting &greeting_;

};     // class XmlGreetingVisitor

class XmlStartVisitor : public TiXmlVisitor {
public:
	XmlStartVisitor(start &s) : start_(s) {}
	virtual ~XmlStartVisitor() {}

	virtual bool VisitEnter(const TiXmlDocument &doc)
	{
		if (const TiXmlElement *element = doc.RootElement()) {
			return ("start" == element->ValueStr());
		}
		return false;
	}

	virtual bool VisitEnter(const TiXmlElement &element, const TiXmlAttribute *attribute)
	{
		if ("profile" == element.ValueStr()) {
			for (; attribute; attribute = attribute->Next()) {
				if (std::string("uri") == attribute->Name()) {
					beep::profile myProfile;
					myProfile.set_uri(attribute->ValueStr());
					if (element.GetText()) {
						beep::message msg;
						/// \todo parse the text for real
						msg.set_content(element.GetText());
					}
					start_.push_back_profile(myProfile);
				}
			}
		} else if ("start" == element.ValueStr()) {
			for (; attribute; attribute = attribute->Next()) {
				if (std::string("number") == attribute->Name()) {
					start_.set_number(attribute->IntValue());
				} else if (std::string("serverName") == attribute->Name()) {
					start_.set_server_name(attribute->ValueStr());
				}
			}
		}
		return true;
	}
private:
	start &start_;

};     // class XmlGreetingVisitor

class XmlCloseVisitor : public TiXmlVisitor {
public:
	XmlCloseVisitor(close &c) : close_(c) {}
	virtual ~XmlCloseVisitor() {}

	virtual bool VisitEnter(const TiXmlDocument &doc)
	{
		if (const TiXmlElement *element = doc.RootElement()) {
			return ("close" == element->ValueStr());
		}
		return false;
	}

	virtual bool VisitEnter(const TiXmlElement &element, const TiXmlAttribute *attribute)
	{
		if ("close" == element.ValueStr()) {
			for (; attribute; attribute = attribute->Next()) {
				if (std::string("number") == attribute->Name()) {
					close_.set_number(attribute->IntValue());
				} else if (std::string("code") == attribute->Name()) {
					close_.set_code(attribute->IntValue());
				}
			}
		}
		return true;
	}
private:
	close &close_;

};     // class XmlVisitor

}      // namespace cmp
}      // namespace beep

namespace std {

// for now, always use tinyxml to encode the greeting
ostream&
operator<<(ostream &strm, const beep::cmp::greeting &protocol)
{
	if (strm) {
		TiXmlElement root("greeting");
		typedef beep::cmp::greeting::uri_const_iterator iterator;
		for(iterator i = protocol.begin(); i != protocol.end(); ++i) {
			TiXmlElement aProfile("profile");
			aProfile.SetAttribute("uri", *i);
			/// \todo Set the profile "encoding" (if required/allowed)
			TiXmlNode *result = root.InsertEndChild(aProfile);
			if (!result) {
				/// \todo handle error
			}
			assert(result);
		}
		/// \todo add a "features" attribute for optional feature
		/// \todo add a "localize" attribute for each language token
		strm << root;
	}
	return strm;
}

istream&
operator>>(istream &strm, beep::cmp::greeting &protocol)
{
	if (strm) {
		TiXmlDocument doc;
		strm >> doc;
		beep::cmp::XmlGreetingVisitor visitor(protocol);
		if (!doc.Accept(&visitor)) {
			strm.setstate(ios::badbit);
		}
	}
	return strm;
}

ostream&
operator<<(ostream &strm, const beep::cmp::start &start)
{
	if (strm) {
		TiXmlElement root("start");
		root.SetAttribute("number", start.get_number());
		if (!start.get_server_name().empty()) {
			root.SetAttribute("serverName", start.get_server_name());
		}
		typedef beep::cmp::start::profile_const_iterator iterator;
		for(iterator i = start.profiles_begin(); i != start.profiles_end(); ++i) {
			TiXmlElement aProfile("profile");
			aProfile.SetAttribute("uri", i->uri());
			/// \todo Set the profile "encoding"
			/// \todo Set the profile initialization content
			TiXmlNode *result = root.InsertEndChild(aProfile);
			if (!result) {
				/// \todo handle error
			}
			assert(result);
		}
		strm << root;
	}
	return strm;
}

istream&
operator>>(istream &strm, beep::cmp::start &start)
{
	if (strm) {
		TiXmlDocument doc;
		strm >> doc;
		beep::cmp::XmlStartVisitor visitor(start);
		if (!doc.Accept(&visitor)) {
			strm.setstate(ios::badbit);
		}
	}
	return strm;
}

ostream&
operator<<(ostream &strm, const beep::cmp::close &close)
{
	if (strm) {
		TiXmlElement root("close");
		root.SetAttribute("number", close.number());
		root.SetAttribute("code", close.code());
		strm << root;
	}
	return strm;
}

istream&
operator>>(istream &strm, beep::cmp::close &close)
{
	if (strm) {
		TiXmlDocument doc;
		strm >> doc;
		beep::cmp::XmlCloseVisitor visitor(close);
		if (!doc.Accept(&visitor)) {
			strm.setstate(ios::badbit);
		}
	}
	return strm;
}

ostream&
operator<<(ostream &strm, const beep::cmp::ok&)
{
	if (strm) {
		strm << "<ok />";
	}
	return strm;
}

ostream&
operator<<(ostream &strm, const beep::cmp::error &error)
{
	if (strm) {
		TiXmlElement root("error");
		root.SetAttribute("code", error.code());
		TiXmlText text(error.description());
		root.InsertEndChild(text);
		strm << root;
	}
	return strm;
}

}      // namespace std

namespace beep {

class channel_manager {
public:
	channel_manager()
		: zero_(0)
		, chnum_()
		, guess_(0)
	{
		chnum_.insert(0u);
	}

	const channel &get_tuning_channel() const { return zero_; }
	channel &get_tuning_channel() { return zero_; }

	template <typename OutputIterator>
	void get_profiles(const message &greeting_msg, OutputIterator out)
	{
		using std::istringstream;
		using std::transform;

		istringstream strm(greeting_msg.content());
		cmp::greeting g_protocol;
		if (strm >> g_protocol) {
			transform(g_protocol.begin(), g_protocol.end(), out, cmp::detail::uri_to_profile);
		} else {
			throw std::runtime_error("could not decode a greeting message.");
		}
	}

	template <typename FwdIterator>
	void
	get_greeting_message(const FwdIterator first_profile, const FwdIterator last_profile,
						 message &greeting_msg) const
	{
		using std::ostringstream;

		greeting_msg.set_mime(mime::beep_xml());
		greeting_msg.set_type(message::rpy);
		cmp::greeting g_protocol(first_profile, last_profile);
		ostringstream strm;
		if (strm << g_protocol) {
			greeting_msg.set_content(strm.str());
		} else {
			throw std::runtime_error("could not encode a greeting message.");
		}
	}

	bool channel_in_use(const unsigned int channel) const
	{
		return chnum_.count(channel) > 0;
	}

	/// To avoid conflict in assigning channel numbers when requesting the
	/// creation of a channel, BEEP peers acting in the initiating role use
	/// only positive integers that are odd-numbered; similarly, BEEP peers
	/// acting in the listening role use only positive integers that are
	/// even-numbered.
	unsigned int start_channel(const role r, const std::string &name,
							   const profile &p, message &msg)
	{
		using std::ostringstream;

		unsigned int number = guess_;
		if (chnum_.insert(number).second) {
			guess_ = number + 2;
		} else {
			number = get_next_channel(r);
			if (!chnum_.insert(number).second) {
				throw std::runtime_error("could not find a channel number!");
			}
		}
		msg.set_mime(mime::beep_xml());
		msg.set_type(message::msg);
		cmp::start start;
		start.set_number(number);
		start.set_server_name(name);
		start.push_back_profile(p);
		ostringstream strm;
		strm << start;
		msg.set_content(strm.str());
		return number;
	}

	void close_channel(const unsigned int number, const reply_code::rc_enum rc,
					   message &msg)
	{
		using std::ostringstream;
		msg.set_mime(mime::beep_xml());
		if (chnum_.erase(number)) {
			msg.set_type(message::msg);
			cmp::close close;
			close.set_number(number);
			close.set_code(rc);
			ostringstream strm;
			strm << close;
			msg.set_content(strm.str());
		} else {
			throw std::runtime_error("The requested channel was not in use.");
		}
	}

	/// \return Accepted channel number, zero indicates an error
	template <typename FwdIterator>
	unsigned int accept_start(const message &start_msg,
							  FwdIterator first_profile,
							  const FwdIterator last_profile,
							  profile &acceptedProfile,
							  message &response)
	{
		using std::istringstream;
		using std::ostringstream;
		using std::find;
		using std::copy;
		using std::ostream_iterator;
		using std::iterator_traits;

		unsigned int channel = 0;
		cmp::start start;
		istringstream strm(start_msg.content());
		ostringstream ostrm;
		response.set_mime(mime::beep_xml());
		if (strm >> start) {
			channel = start.get_number();
			bool match_profile = false;
			if (chnum_.insert(channel).second) {
				typedef cmp::start::profile_const_iterator const_iterator;
				for (const_iterator i = start.profiles_begin(); i != start.profiles_end() && !match_profile; ++i) {
					if (find(first_profile, last_profile, *i) != last_profile) {
						acceptedProfile = *i;
						match_profile = true;
					}
				}
				if (match_profile) {
					response.set_type(message::rpy);
					cmp::ok ok;
					ostrm << ok;
				} else {
					chnum_.erase(channel);
					channel = 0;
					response.set_type(message::err);
					ostringstream estrm;
					estrm << "The specified profile(s) are not supported. "
						"This listener supports the following profiles: ";
					typedef typename iterator_traits<FwdIterator>::value_type value_type;
					copy(first_profile, last_profile, ostream_iterator<value_type>(estrm, ", "));
					cmp::error error(reply_code::requested_action_not_accepted,
									 estrm.str());
					ostrm << error;
				}
			} else {
				response.set_type(message::err);
				ostringstream estrm;
				estrm << "The requested channel (" << channel
					  << ") is already in use.";
				cmp::error error(reply_code::requested_action_not_accepted,
								 estrm.str());
				ostrm << error;
			}
		} else {
			response.set_type(message::err);
			cmp::error error(reply_code::general_syntax_error,
							 "The 'start' message could not be decoded.");
			ostrm << error;
		}
		response.set_content(ostrm.str());
		return channel;
	}

	bool close_channel(const message &close_msg, message &response)
	{
		using std::istringstream;
		using std::ostringstream;

		bool is_ok = false;
		cmp::close close;
		istringstream strm(close_msg.content());
		ostringstream ostrm;
		response.set_mime(mime::beep_xml());
		if (is_ok = strm >> close) {
			response.set_type(message::rpy);
			cmp::ok ok;
			ostrm << ok;
		} else {
			response.set_type(message::err);
			cmp::error error(reply_code::general_syntax_error,
							 "The 'close' message could not be decoded.");
			ostrm << error;
		}
		response.set_content(ostrm.str());
		return is_ok;
	}
private:
	typedef std::set<unsigned int> ch_set;
	channel      zero_; ///< used for adding/removing subsequent channels
	ch_set       chnum_;
	unsigned int guess_; // guess at next channel number

	static unsigned int get_first_number(const role r)
	{
		unsigned int number = 0;
		switch (r) {
		case initiating_role:
			number = 1;
			break;
		case listening_role:
			number = 2;
			break;
		default:
			throw std::runtime_error("Unknown beep role in the channel manager");
		}
		return number;
	}

	unsigned int get_next_channel(const role r)
	{
		unsigned int number = 0;
		if (chnum_.empty()) {
			number = get_first_number(r);
		} else {
			number = *chnum_.rbegin();
			if (!number) {
				number = get_first_number(r);
			}
		}
		return number;
	}
};

}      // namespace beep
#endif // BEEP_CHANNEL_MANAGER_HEAD
