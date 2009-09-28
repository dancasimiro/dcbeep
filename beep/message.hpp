/// \file  message.hpp
/// \brief BEEP message
///
/// UNCLASSIFIED
#ifndef BEEP_MESSAGE_HEAD
#define BEEP_MESSAGE_HEAD 1

#include <cstddef>
#include <string>

namespace beep {

class mime {
public:
	mime()
		: type_("Content-Type: application/octet-stream")
		, encoding_("binary")
	{
	}

	const std::string &content_type() const { return type_; }
	void set_content_type(const std::string &ct)
	{
		type_ = "Content-Type: ";
		type_ += ct;
	}
private:
	std::string type_;
	std::string encoding_;
};

class message {
public:
	typedef std::string content_type;
	typedef std::size_t size_type;

	message()
		: mime_()
		, content_()
		, payload_()
	{
	}

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

	const content_type &payload() const { return payload_; }
	size_type payload_size() const { return payload_.size(); }
private:
	mime         mime_;
	content_type content_;
	content_type payload_;

	void update_payload()
	{
		static const std::string sep = "\r\n\r\n";
		payload_ = mime_.content_type() + sep + content_;
	}
};     // class message

}      // namespace beep
#endif // BEEP_MESSAGE_HEAD
