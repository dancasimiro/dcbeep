/// \file  message.hpp
/// \brief BEEP message
///
/// UNCLASSIFIED
#ifndef BEEP_MESSAGE_HEAD
#define BEEP_MESSAGE_HEAD 1

#include <cstddef>
#include <string>

#include "channel.hpp"

namespace beep {

class mime {
public:
	mime();
	mime(const std::string &c, const std::string &e);

	const std::string &get_content_type() const { return type_; }
	void set_content_type(const std::string &ct);
public:
	static const mime &beep_xml();
private:
	std::string type_;
	std::string encoding_;
};

class message {
public:
	typedef std::string content_type;
	typedef std::size_t size_type;

	message();

	void set_mime(const mime &m)
	{
		mime_ = m;
		update_payload();
	}

	void set_content(const content_type &c)
	{
		content_ = c;
		update_payload();
	}

	const mime &get_mime() const { return mime_; }
	const content_type &get_content() const { return content_; }

	const content_type &get_payload() const { return payload_; }
	size_type get_payload_size() const { return payload_.size(); }

	void set_type(const unsigned int t) { type_ = t; }
	unsigned int get_type() const { return type_; }

	void set_channel(const channel &c) { channel_ = c; }
	const channel &get_channel() const { return channel_; }
private:
	mime         mime_;
	content_type content_;
	content_type payload_;
	unsigned int type_;
	channel      channel_;

	void update_payload();
};     // class message

bool operator==(const message &lhs, const message &rhs);
bool operator==(const mime &lhs, const mime &rhs);

}      // namespace beep
#endif // BEEP_MESSAGE_HEAD
