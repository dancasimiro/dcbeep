/// \file message.cpp
/// \brief compile message object
///
/// UNCLASSIFIED
#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <string>
#include "message.hpp"

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
	type_ = std::string("Content-Type: ") + ct;
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
	, payload_()
	, type_(0)
	, channel_()
{
}

void
message::update_payload()
{
	static const std::string sep = "\r\n\r\n";
	payload_ = mime_.get_content_type() + sep + content_;
}

bool operator==(const message &lhs, const message &rhs)
{
	return lhs.get_payload() == rhs.get_payload();
}

bool operator==(const mime &lhs, const mime &rhs)
{
	return lhs.get_content_type() == rhs.get_content_type();
}

} // namespace beep
