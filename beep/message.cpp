/// \file message.cpp
/// \brief compile message object
///
/// UNCLASSIFIED
#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <string>
#include <boost/variant.hpp>
#include "frame.hpp"
#include "message.hpp"
#include "message-generator.hpp"

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix.hpp>

namespace beep {

mime::mime()
	: type_("Content-Type: application/octet-stream")
	, encoding_("binary")
{
}

mime::mime(const std::string &c, const std::string &e)
	: type_(std::string("Content-Type: ") + c)
	, encoding_(e)
{
}

void
mime::set_content_type(const std::string &ct)
{
	type_ = "Content-Type:";
	if (!ct.empty() && ct[0] != ' ') {
		type_ += ' ';
	}
	type_ += ct;
}

const mime &
mime::beep_xml()
{
	static const mime m("application/beep+xml", "binary");
	return m;
}

message::message()
	: mime_()
	, content_()
	, type_(0)
	, channel_()
{
}

void
message::set_payload(const std::string &p)
{
	using boost::spirit::qi::parse;
	using boost::spirit::qi::print;
	using boost::spirit::qi::no_case;
	using boost::phoenix::ref;
	using boost::phoenix::assign;
	using boost::phoenix::begin;
	using boost::phoenix::end;
	using boost::spirit::qi::_1;

	std::string::const_iterator i = p.begin();
	const std::string::const_iterator i_end = p.end();
	std::string mime_content_type = mime_.get_content_type();
	if (parse(i, i_end,
			  no_case["Content-Type:"]
			  >> (+print)[assign(ref(mime_content_type), begin(_1), end(_1))]
			  >> "\r\n\r\n"
			  )) {
		mime_.set_content_type(mime_content_type);
		content_.assign(i, i_end);
	} else {
		content_ = p;
	}
}

std::string
message::get_payload() const
{
	static const std::string sep = "\r\n\r\n";
	return mime_.get_content_type() + sep + content_;
}

bool operator!=(const message &lhs, const message &rhs)
{
	return
		lhs.get_channel() != rhs.get_channel()
		|| lhs.get_type() != rhs.get_type()
		|| lhs.get_mime() != rhs.get_mime()
		|| lhs.get_content() != rhs.get_content()
		;
}

bool operator==(const message &lhs, const message &rhs)
{
	return !(operator!=(lhs, rhs));
}

bool
operator<(const message &lhs, const message &rhs)
{
	return
		lhs.get_type() < rhs.get_type()
		&& lhs.get_channel() < rhs.get_channel()
		;
}

bool operator!=(const mime &lhs, const mime &rhs)
{
	return lhs.get_content_type() != rhs.get_content_type();
}

bool operator==(const mime &lhs, const mime &rhs)
{
	return !(operator!=(lhs, rhs));
}

std::ostream&
operator<<(std::ostream &strm, const mime &mm)
{
	if (strm) {
		if (mm == beep::mime::beep_xml()) {
			strm << "BEEP+XML MIME";
		} else {
			strm << "Unknown MIME (" << mm.get_content_type() << ")";
		}
	}
	return strm;
}

std::ostream&
operator<<(std::ostream &strm, const message &msg)
{
	if (strm) {
		strm << "BEEP [" << msg.get_mime() << "] message";
	}
	return strm;
}

message &
operator+=(message &lhs, const message &rhs)
{
	if (lhs.get_channel() != rhs.get_channel()) {
		throw std::runtime_error("The messages cannot be accumulated; different channels!");
	}
	if (lhs.get_type() != rhs.get_type()) {
		throw std::runtime_error("the messages cannot be accumulated; different types!");
	}
	lhs.set_content(lhs.get_content() + rhs.get_content());
	return lhs;
}

message
operator+(const message &lhs, const message &rhs)
{
	message output = lhs;
	output += rhs;
	return output;
}

namespace detail {
using std::make_pair;

/// \return a message object and a bool indicating if the frame completes the message
class frame_aggregator : public boost::static_visitor<std::pair<message, bool> > {
public:
	/// MSG, RPY, ANS, NUL, and ERR frames
	template <core_message_types CoreFrameType>
	std::pair<message, bool> operator()(const basic_frame<CoreFrameType> &frm) const
	{
		message out;
		out.set_type(frm.header());
		out.set_channel(channel(frm.channel, frm.message));
		out.set_payload(frm.payload);
		return make_pair(out, !frm.more);
	}

	std::pair<message, bool> operator()(const seq_frame &/*seq*/) const
	{
		assert(false);
		throw std::runtime_error("SEQ frames are not supported at this location");
		return make_pair(message(), false);
	}
};
} // namespace detail

message_compiler::message_compiler()
	: pending_()
{
}

bool
message_compiler::operator()(const frame &in, message &out)
{
	typedef std::set<message>::iterator iterator;
	const std::pair<message, bool> result =
		apply_visitor(detail::frame_aggregator(), in);
	iterator i = pending_.find(result.first);
	if (i != pending_.end()) {
		const message combined = *i + result.first;
		pending_.erase(i);
		const std::pair<iterator, bool> insert_result = pending_.insert(combined);
		if (!insert_result.second) {
			throw std::runtime_error("The updated message could not be updated in the set.");
		}
		i = insert_result.first;
	}
	// if message is complete...
	if (result.second) {
		if (i != pending_.end()) {
			out = *i;
			pending_.erase(i);
		} else {
			out = result.first;
		}
		return true;
	} else if (i == pending_.end()) {
		pending_.insert(result.first);
	}
	return false;
}

} // namespace beep
