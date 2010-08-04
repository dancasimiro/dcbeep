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
#include <limits>

#include "role.hpp"
#include "frame.hpp" // for core_message_types definition
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

inline
profile uri_to_profile(const std::string &uri)
{
	profile p;
	p.set_uri(uri);
	return p;
}

inline
bool message_has_element_named(const message &msg, const std::string &name)
{
	bool is = false;
	using std::getline;
	using std::string;
	using std::istringstream;
	using std::transform;

	istringstream strm(msg.get_content());
	string line;
	while (!is && getline(strm, line)) {
		transform(line.begin(), line.end(), line.begin(), ::tolower);
		is = (line.find(name) != string::npos);
	}
	return is;
}

}      // namespace detail

inline
bool is_greeting_message(const message &msg)
{
	return detail::message_has_element_named(msg, "greeting");
}

inline
bool is_start_message(const message &msg)
{
	return detail::message_has_element_named(msg, "start");
}

inline
bool is_close_message(const message &msg)
{
	return detail::message_has_element_named(msg, "close");
}

inline
bool is_ok_message(const message &msg)
{
	return detail::message_has_element_named(msg, "ok");
}

inline
bool is_profile_message(const message &msg)
{
	return detail::message_has_element_named(msg, "profile");
}

inline
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

	greeting(const profile &my_profile)
		: uris_()
	{
		uris_.push_back(my_profile.uri());
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

	unsigned int number() const { return channel_; }
	void set_number(const unsigned int c) { channel_ = c; }

	const std::string &server_name() const { return server_; }
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

class XmlErrorVisitor : public TiXmlVisitor {
public:
	XmlErrorVisitor(error &e) : error_(e) {}
	virtual ~XmlErrorVisitor() {}

	virtual bool VisitEnter(const TiXmlDocument &doc)
	{
		if (const TiXmlElement *element = doc.RootElement()) {
			return ("error" == element->ValueStr());
		}
		return false;
	}

	virtual bool VisitEnter(const TiXmlElement &element, const TiXmlAttribute *attribute)
	{
		using std::istringstream;
		using namespace beep::reply_code;
		if ("error" == element.ValueStr()) {
			for (; attribute; attribute = attribute->Next()) {
				if (std::string("code") == attribute->Name()) {
					const int raw_rc = attribute->IntValue();
					assert(raw_rc == success ||
						   raw_rc == service_not_available ||
						   raw_rc == requested_action_not_taken ||
						   raw_rc == requested_action_aborted ||
						   raw_rc == temporary_authentication_failure ||
						   raw_rc == general_syntax_error ||
						   raw_rc == syntax_error_in_parameters ||
						   raw_rc == parameter_not_implemented ||
						   raw_rc == authentication_required ||
						   raw_rc == authentication_mechanism_insufficient ||
						   raw_rc == authentication_failure ||
						   raw_rc == action_not_authorized_for_user ||
						   raw_rc == authentication_mechanism_requires_encryption ||
						   raw_rc == requested_action_not_accepted ||
						   raw_rc == parameter_invalid ||
						   raw_rc == transaction_failed);
					error_.set_code(static_cast<rc_enum>(raw_rc));
				}
			}
			error_.set_description(element.GetText());
		}
		return true;
	}
private:
	error &error_;

};     // class XmlErrorVisitor

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
inline
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

inline
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

inline
ostream&
operator<<(ostream &strm, const beep::cmp::start &start)
{
	if (strm) {
		TiXmlElement root("start");
		root.SetAttribute("number", start.number());
		if (!start.server_name().empty()) {
			root.SetAttribute("serverName", start.server_name());
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

inline
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

inline
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

inline
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

inline
ostream&
operator<<(ostream &strm, const beep::cmp::ok&)
{
	if (strm) {
		strm << "<ok />";
	}
	return strm;
}

inline
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

inline
istream&
operator>>(istream &strm, beep::cmp::error &error)
{
	if (strm) {
		TiXmlDocument doc;
		strm >> doc;
		beep::cmp::XmlErrorVisitor visitor(error);
		if (!doc.Accept(&visitor)) {
			strm.setstate(ios::badbit);
		}
	}
	return strm;
}

}      // namespace std

namespace beep {

inline
boost::system::system_error
make_error(const message &msg)
{
	using std::runtime_error;
	using std::istringstream;
	using boost::system::error_code;
	using boost::system::system_error;

	if (msg.get_type() != ERR) {
		throw runtime_error("An error code cannot be created from the given message.");
	}
	cmp::error myError;
	istringstream strm(msg.get_content());
	if (strm >> myError) {
		const error_code ec(myError.code(), beep::beep_category);
		return system_error(ec, myError.description());
	} else {
		throw runtime_error("could not decode the error message.");
	}
}

class channel_manager {
public:
	channel_manager()
		: zero_(0)
		, chnum_()
		, guess_(0)
	{
		chnum_.insert(0u);
	}

	const channel &tuning_channel() const { return zero_; }
	channel &tuning_channel() { return zero_; }

	template <typename OutputIterator>
	void copy_profiles(const message &greeting_msg, OutputIterator out)
	{
		using std::istringstream;
		using std::transform;

		istringstream strm(greeting_msg.get_content());
		cmp::greeting g_protocol;
		if (strm >> g_protocol) {
			transform(g_protocol.begin(), g_protocol.end(), out, cmp::detail::uri_to_profile);
		} else {
			throw std::runtime_error("could not decode a greeting message.");
		}
	}

	template <typename FwdIterator>
	void
	greeting_message(const FwdIterator first_profile, const FwdIterator last_profile,
					 message &greeting_msg) const
	{
		using std::ostringstream;

		greeting_msg.set_mime(mime::beep_xml());
		greeting_msg.set_type(RPY);
		cmp::greeting g_protocol(first_profile, last_profile);
		ostringstream strm;
		if (strm << g_protocol) {
			greeting_msg.set_content(strm.str());
		} else {
			throw std::runtime_error("could not encode a greeting message.");
		}
	}

	/// \brief test if a channel is active
	///
	/// First, test if the channel number represents the tuning channel. If not,
	/// check if the number is currently in use.
	///
	/// \return True if the channel number is currently active.
	bool channel_in_use(const unsigned int channel) const
	{
		assert(zero_.get_number() == 0u);
		return (channel == zero_.get_number()) || chnum_.count(channel) > 0;
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
		if (!chnum_.insert(number).second) {
			number = get_next_channel(r);
			if (!chnum_.insert(number).second) {
				throw std::runtime_error("could not find a channel number!");
			}
		}
		msg.set_mime(mime::beep_xml());
		msg.set_type(MSG);
		cmp::start start;
		start.set_number(number);
		start.set_server_name(name);
		start.push_back_profile(p);
		ostringstream strm;
		strm << start;
		msg.set_content(strm.str());
		guess_ = number + 2;
		return number;
	}

	void close_channel(const unsigned int number, const reply_code::rc_enum rc,
					   message &msg)
	{
		using std::ostringstream;
		msg.set_mime(mime::beep_xml());
		if (number > 0 && !chnum_.erase(number)) {
			throw std::runtime_error("The requested channel was not in use.");
		}
		msg.set_type(MSG);
		cmp::close close;
		close.set_number(number);
		close.set_code(rc);
		ostringstream strm;
		strm << close;
		msg.set_content(strm.str());
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
		istringstream strm(start_msg.get_content());
		ostringstream ostrm;
		response.set_mime(mime::beep_xml());
		if (strm >> start) {
			channel = start.number();
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
					response.set_type(RPY);
					TiXmlElement aProfile("profile");
					aProfile.SetAttribute("uri", acceptedProfile.uri());
					/// \todo Set the profile "encoding" (if required/allowed)
					/// \todo add a "features" attribute for optional feature
					/// \todo add a "localize" attribute for each language token
					ostrm << aProfile;
				} else {
					chnum_.erase(channel);
					channel = 0;
					response.set_type(ERR);
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
				response.set_type(ERR);
				ostringstream estrm;
				estrm << "The requested channel (" << channel
					  << ") is already in use.";
				cmp::error error(reply_code::requested_action_not_accepted,
								 estrm.str());
				ostrm << error;
			}
		} else {
			response.set_type(ERR);
			cmp::error error(reply_code::general_syntax_error,
							 "The 'start' message could not be decoded.");
			ostrm << error;
		}
		response.set_content(ostrm.str());
		return channel;
	}

	unsigned close_channel(const message &close_msg, message &response)
	{
		using std::istringstream;
		using std::ostringstream;
		using std::numeric_limits;

		response.set_mime(mime::beep_xml());

		cmp::close close;
		istringstream strm(close_msg.get_content());
		if (!(strm >> close)) {
			response.set_type(ERR);
			cmp::error error(reply_code::general_syntax_error,
							 "The 'close' message could not be decoded.");
			ostringstream ostrm;
			ostrm << error;
			response.set_content(ostrm.str());
			return numeric_limits<unsigned>::max();
		}
		const unsigned ch_num = close.number();
		if ((ch_num == zero_.get_number()) || chnum_.erase(ch_num)) {
			response.set_type(RPY);
			cmp::ok ok;
			ostringstream ostrm;
			ostrm << ok;
			response.set_content(ostrm.str());
		} else {
			response.set_type(ERR);
			cmp::error error(reply_code::parameter_invalid,
							 "Channel closure failed because the referenced channel number is not recognized.");
			ostringstream ostrm;
			ostrm << error;
			response.set_content(ostrm.str());
			return numeric_limits<unsigned>::max();
		}
		return ch_num;
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
			} else {
				number += 2;
			}
		}
		return number;
	}
};

}      // namespace beep
#endif // BEEP_CHANNEL_MANAGER_HEAD
