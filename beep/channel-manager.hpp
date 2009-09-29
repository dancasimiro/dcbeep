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
			aProfile.SetAttribute("uri", i->get_uri());
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
		greeting_msg.set_type(message::RPY);
		cmp::greeting g_protocol(first_profile, last_profile);
		ostringstream strm;
		if (strm << g_protocol) {
			greeting_msg.set_content(strm.str());
		} else {
			throw std::runtime_error("could not encode a greeting message.");
		}
	}

	/// To avoid conflict in assigning channel numbers when requesting the
	/// creation of a channel, BEEP peers acting in the initiating role use
	/// only positive integers that are odd-numbered; similarly, BEEP peers
	/// acting in the listening role use only positive integers that are
	/// even-numbered.
	unsigned int get_start_message(const role r,
								   const std::string &name,
								   const profile &p,
								   message &msg)
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
		msg.set_type(message::MSG);
		cmp::start start;
		start.set_number(number);
		start.set_server_name(name);
		start.push_back_profile(p);
		ostringstream strm;
		strm << start;
		msg.set_content(strm.str());
		return number;
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
