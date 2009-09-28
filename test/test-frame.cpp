/// \file test-frame.cpp
///
/// UNCLASSIFIED
//#define BOOST_SPIRIT_DEBUG
#include <gtest/gtest.h>

#include <string>
#include <boost/spirit/include/classic.hpp>
#include "beep/frame.hpp"
#include "beep/frame-stream.hpp"

using namespace BOOST_SPIRIT_CLASSIC_NS;

TEST(SpaceParser, Valid)
{
	static const std::string content = " ";
	typedef scanner<std::string::const_iterator> scanner_type;
	std::size_t size = 0;
	beep::frame f;
	beep::frame_syntax syntax(f, size);
	beep::frame_syntax::definition<scanner_type> def(syntax);
	EXPECT_NO_THROW(parse(content.begin(), content.end(), def.sp));
}

TEST(CarriageReturnLineFeedParser, Valid)
{
	static const std::string content = "\r\n";
	typedef scanner<std::string::const_iterator> scanner_type;
	std::size_t size = 0;
	beep::frame f;
	beep::frame_syntax syntax(f, size);
	beep::frame_syntax::definition<scanner_type> def(syntax);
	EXPECT_NO_THROW(parse(content.begin(), content.end(), def.crlf));
}

TEST(ChannelParser, Valid)
{
	static const std::string content = "10";
	typedef scanner<std::string::const_iterator> scanner_type;
	std::size_t size = 0;
	beep::frame aFrame;
	beep::frame_syntax syntax(aFrame, size);
	beep::frame_syntax::definition<scanner_type> def(syntax);
	EXPECT_NO_THROW(parse(content.begin(), content.end(), def.channel));
	EXPECT_EQ(10u, aFrame.channel());
}

TEST(MessageNumberParser, Valid)
{
	static const std::string content = "3";
	typedef scanner<std::string::const_iterator> scanner_type;
	std::size_t size = 0;
	beep::frame aFrame;
	beep::frame_syntax syntax(aFrame, size);
	beep::frame_syntax::definition<scanner_type> def(syntax);
	EXPECT_NO_THROW(parse(content.begin(), content.end(), def.msgno));
	EXPECT_EQ(3u, aFrame.message());
}

TEST(MoreParser, Valid)
{
	static const std::string content = ".";
	typedef scanner<std::string::const_iterator> scanner_type;
	std::size_t size = 0;
	beep::frame aFrame;
	beep::frame_syntax syntax(aFrame, size);
	beep::frame_syntax::definition<scanner_type> def(syntax);
	EXPECT_NO_THROW(parse(content.begin(), content.end(), def.more));
	EXPECT_FALSE(aFrame.more());
}

TEST(SequenceParser, Valid)
{
	static const std::string content = "6";
	typedef scanner<std::string::const_iterator> scanner_type;
	std::size_t size = 0;
	beep::frame aFrame;
	beep::frame_syntax syntax(aFrame, size);
	beep::frame_syntax::definition<scanner_type> def(syntax);
	EXPECT_NO_THROW(parse(content.begin(), content.end(), def.seqno));
	EXPECT_EQ(6u, aFrame.sequence());
}

TEST(SizeParser, Valid)
{
	static const std::string content = "120";
	typedef scanner<std::string::const_iterator> scanner_type;
	std::size_t mySize = 0;
	beep::frame f;
	beep::frame_syntax syntax(f, mySize);
	beep::frame_syntax::definition<scanner_type> def(syntax);
	EXPECT_NO_THROW(parse(content.begin(), content.end(), def.size));
	EXPECT_EQ(120u, mySize);
}

TEST(AnswerParser, Valid)
{
	static const std::string content = "19";
	typedef scanner<std::string::const_iterator> scanner_type;
	std::size_t size = 0;
	beep::frame aFrame;
	beep::frame_syntax syntax(aFrame, size);
	beep::frame_syntax::definition<scanner_type> def(syntax);
	EXPECT_NO_THROW(parse(content.begin(), content.end(), def.ansno));
	EXPECT_EQ(19u, aFrame.answer());
}

TEST(CommonParser, Valid)
{
	static const std::string content = "19 2 . 3 12\r\n";
	typedef scanner<std::string::const_iterator> scanner_type;
	std::size_t size = 0;
	beep::frame aFrame;
	beep::frame_syntax syntax(aFrame, size);
	beep::frame_syntax::definition<scanner_type> def(syntax);
	EXPECT_NO_THROW(parse(content.begin(), content.end(), def.common));
	EXPECT_EQ(19u, aFrame.channel());
	EXPECT_EQ(2u, aFrame.message());
	EXPECT_FALSE(aFrame.more());
	EXPECT_EQ(3u, aFrame.sequence());
}

TEST(MessageFrameHeader, Valid)
{
	static const std::string content = "MSG 19 2 . 3 12\r\n";
	typedef scanner<std::string::const_iterator> scanner_type;
	beep::frame aFrame;
	std::size_t payload_size = 0;
	beep::frame_syntax syntax(aFrame, payload_size);
	beep::frame_syntax::definition<scanner_type> def(syntax);
	EXPECT_NO_THROW(parse(content.begin(), content.end(), def.message_frame));
	EXPECT_EQ("MSG", aFrame.header());
	EXPECT_EQ(19u, aFrame.channel());
	EXPECT_EQ(2u, aFrame.message());
	EXPECT_FALSE(aFrame.more());
	EXPECT_EQ(3u, aFrame.sequence());
}

TEST(ReplyHeader, Valid)
{
	static const std::string content = "RPY 19 2 . 3 120\r\n";
	typedef scanner<std::string::const_iterator> scanner_type;
	std::size_t size = 0;
	beep::frame aFrame;
	beep::frame_syntax syntax(aFrame, size);
	beep::frame_syntax::definition<scanner_type> def(syntax);
	EXPECT_NO_THROW(parse(content.begin(), content.end(), def.reply_frame));
	EXPECT_EQ("RPY", aFrame.header());
	EXPECT_EQ(19u, aFrame.channel());
	EXPECT_EQ(2u, aFrame.message());
	EXPECT_FALSE(aFrame.more());
	EXPECT_EQ(3u, aFrame.sequence());
}

TEST(AnswerHeader, Valid)
{
	static const std::string content = "ANS 19 2 . 3 120 5\r\n";
	typedef scanner<std::string::const_iterator> scanner_type;
	std::size_t size = 0;
	beep::frame aFrame;
	beep::frame_syntax syntax(aFrame, size);
	beep::frame_syntax::definition<scanner_type> def(syntax);
	EXPECT_NO_THROW(parse(content.begin(), content.end(), def.answer_frame));
	EXPECT_EQ("ANS", aFrame.header());
	EXPECT_EQ(19u, aFrame.channel());
	EXPECT_EQ(2u, aFrame.message());
	EXPECT_FALSE(aFrame.more());
	EXPECT_EQ(3u, aFrame.sequence());
	EXPECT_EQ(5u, aFrame.answer());
}

TEST(ErrorHeader, Valid)
{
	static const std::string content = "ERR 19 2 . 3 120\r\n";
	typedef scanner<std::string::const_iterator> scanner_type;
	std::size_t size = 0;
	beep::frame aFrame;
	beep::frame_syntax syntax(aFrame, size);
	beep::frame_syntax::definition<scanner_type> def(syntax);
	EXPECT_NO_THROW(parse(content.begin(), content.end(), def.error_frame));
	EXPECT_EQ("ERR", aFrame.header());
	EXPECT_EQ(19u, aFrame.channel());
	EXPECT_EQ(2u, aFrame.message());
	EXPECT_FALSE(aFrame.more());
	EXPECT_EQ(3u, aFrame.sequence());
}

TEST(NullHeader, Valid)
{
	static const std::string content = "NUL 19 2 . 3 120\r\n";
	typedef scanner<std::string::const_iterator> scanner_type;
	std::size_t size = 0;
	beep::frame aFrame;
	beep::frame_syntax syntax(aFrame, size);
	beep::frame_syntax::definition<scanner_type> def(syntax);
	EXPECT_NO_THROW(parse(content.begin(), content.end(), def.null_frame));
	EXPECT_EQ("NUL", aFrame.header());
	EXPECT_EQ(19u, aFrame.channel());
	EXPECT_EQ(2u, aFrame.message());
	EXPECT_FALSE(aFrame.more());
	EXPECT_EQ(3u, aFrame.sequence());
}

TEST(MessageHeaderGenericRule, Valid)
{
	static const std::string content = "MSG 19 2 . 3 120\r\n";
	typedef scanner<std::string::const_iterator> scanner_type;
	std::size_t size = 0;
	beep::frame aFrame;
	beep::frame_syntax syntax(aFrame, size);
	beep::frame_syntax::definition<scanner_type> def(syntax);
	EXPECT_NO_THROW(parse(content.begin(), content.end(), def.supported_frame));
	EXPECT_EQ("MSG", aFrame.header());
	EXPECT_EQ(19u, aFrame.channel());
	EXPECT_EQ(2u, aFrame.message());
	EXPECT_FALSE(aFrame.more());
	EXPECT_EQ(3u, aFrame.sequence());
}

TEST(ReplyHeaderGenericRule, Valid)
{
	static const std::string content = "RPY 19 2 . 3 120\r\n";
	typedef scanner<std::string::const_iterator> scanner_type;
	std::size_t size = 0;
	beep::frame aFrame;
	beep::frame_syntax syntax(aFrame, size);
	beep::frame_syntax::definition<scanner_type> def(syntax);
	EXPECT_NO_THROW(parse(content.begin(), content.end(), def.supported_frame));
	EXPECT_EQ("RPY", aFrame.header());
	EXPECT_EQ(19u, aFrame.channel());
	EXPECT_EQ(2u, aFrame.message());
	EXPECT_FALSE(aFrame.more());
	EXPECT_EQ(3u, aFrame.sequence());
}

TEST(AnswerHeaderGenericRule, Valid)
{
	static const std::string content = "ANS 19 2 . 3 120 5\r\n";
	typedef scanner<std::string::const_iterator> scanner_type;
	std::size_t size = 0;
	beep::frame aFrame;
	beep::frame_syntax syntax(aFrame, size);
	beep::frame_syntax::definition<scanner_type> def(syntax);
	EXPECT_NO_THROW(parse(content.begin(), content.end(), def.supported_frame));
	EXPECT_EQ("ANS", aFrame.header());
	EXPECT_EQ(19u, aFrame.channel());
	EXPECT_EQ(2u, aFrame.message());
	EXPECT_FALSE(aFrame.more());
	EXPECT_EQ(3u, aFrame.sequence());
	EXPECT_EQ(5u, aFrame.answer());
}

TEST(ErrorHeaderGenericRule, Valid)
{
	static const std::string content = "ERR 19 2 . 3 120\r\n";
	typedef scanner<std::string::const_iterator> scanner_type;
	std::size_t size = 0;
	beep::frame aFrame;
	beep::frame_syntax syntax(aFrame, size);
	beep::frame_syntax::definition<scanner_type> def(syntax);
	EXPECT_NO_THROW(parse(content.begin(), content.end(), def.supported_frame));
	EXPECT_EQ("ERR", aFrame.header());
	EXPECT_EQ(19u, aFrame.channel());
	EXPECT_EQ(2u, aFrame.message());
	EXPECT_FALSE(aFrame.more());
	EXPECT_EQ(3u, aFrame.sequence());
}

TEST(NullHeaderGenericRule, Valid)
{
	static const std::string content = "NUL 19 2 . 3 120\r\n";
	typedef scanner<std::string::const_iterator> scanner_type;
	std::size_t size = 0;
	beep::frame aFrame;
	beep::frame_syntax syntax(aFrame, size);
	beep::frame_syntax::definition<scanner_type> def(syntax);
	EXPECT_NO_THROW(parse(content.begin(), content.end(), def.supported_frame));
	EXPECT_EQ("NUL", aFrame.header());
	EXPECT_EQ(19u, aFrame.channel());
	EXPECT_EQ(2u, aFrame.message());
	EXPECT_FALSE(aFrame.more());
	EXPECT_EQ(3u, aFrame.sequence());
}

TEST(TrailerParse, Valid)
{
	static const std::string content = "END\r\n";
	typedef scanner<std::string::const_iterator> scanner_type;
	std::size_t size = 0;
	beep::frame f;
	beep::frame_syntax syntax(f, size);
	beep::frame_syntax::definition<scanner_type> def(syntax);
	EXPECT_NO_THROW(parse(content.begin(), content.end(), def.trailer));
}

TEST(PayloadParse, Valid)
{
	static const std::string content = "MSG 19 2 . 3 12\r\nSome PayloadEND\r\n";
	std::size_t size = 0;
	beep::frame aFrame;
	beep::frame_syntax syntax(aFrame, size);
	EXPECT_NO_THROW(parse(content.begin(), content.end(), syntax));
	EXPECT_EQ("MSG", aFrame.header());
	EXPECT_EQ(19u, aFrame.channel());
	EXPECT_EQ(2u, aFrame.message());
	EXPECT_FALSE(aFrame.more());
	EXPECT_EQ(3u, aFrame.sequence());
	EXPECT_EQ("Some Payload", aFrame.payload());
}

///////////////////////////////////////////////////////////////////////////////
// Test valid "message" frames (MSG)
///////////////////////////////////////////////////////////////////////////////
class FrameMessageTest : public testing::Test {
public:
	virtual ~FrameMessageTest() {}
protected:
	virtual void SetUp()
	{
		static const std::string content =
			"MSG 9 1 . 52 120\r\n"
			"Content-Type: application/beep+xml\r\n\r\n" // 38
			"<start number='1'>\r\n" // 20
			"   <profile uri='http://iana.org/beep/SASL/OTP' />\r\n" // 52
			"</start>\r\n" // 10
			"END\r\n";
		size = 0;
		frm = beep::frame();
		beep::frame_syntax syntax(frm, size);
		const parse_info<std::string::const_iterator> pi =
			parse(content.begin(), content.end(), syntax);
		ASSERT_TRUE(pi.full);
		EXPECT_TRUE(pi.stop == content.end());
	}

	virtual void TearDown()
	{
	}

	std::size_t size;
	beep::frame frm;
};

TEST_F(FrameMessageTest, ParseKeyword)
{
	EXPECT_EQ(beep::frame::msg(), frm.header());
}

TEST_F(FrameMessageTest, ParseChannelNumber)
{
	EXPECT_EQ(9U, frm.channel());
}

TEST_F(FrameMessageTest, ParseMessageNumber)
{
	EXPECT_EQ(1U, frm.message());
}

TEST_F(FrameMessageTest, ParseMore)
{
	EXPECT_EQ(false, frm.more());
}

TEST_F(FrameMessageTest, ParseSequenceNumber)
{
	EXPECT_EQ(52U, frm.sequence());
}

TEST_F(FrameMessageTest, ParsePayload)
{
	const std::string payload =
		"Content-Type: application/beep+xml\r\n\r\n" // 38
			"<start number='1'>\r\n" // 20
			"   <profile uri='http://iana.org/beep/SASL/OTP' />\r\n" // 52
			"</start>\r\n" // 10
		;
	EXPECT_EQ(payload, frm.payload());
}

///////////////////////////////////////////////////////////////////////////////
// Test invalid "message" frames (MSG)
///////////////////////////////////////////////////////////////////////////////
class BadFrameMessageTest : public testing::Test {
public:
	virtual ~BadFrameMessageTest() { }
protected:
	virtual void SetUp()
	{
		size = 0;
		frm = beep::frame();
	}

	virtual void TearDown()
	{
	}
	std::size_t size;
	beep::frame frm;
};

TEST_F(BadFrameMessageTest, InvalidParseKeyword)
{
	static const std::string content =
		"DAN 9 1 . 52 120\r\n"
		"Content-Type: application/beep+xml\r\n\r\n" // 38
		"<start number='1'>\r\n" // 20
		"   <profile uri='http://iana.org/beep/SASL/OTP' />\r\n" // 52
		"</start>\r\n" // 10
		"END\r\n";
	beep::frame_syntax syntax(frm, size);
	typedef parser_error<beep::frame_syntax_errors, std::string::const_iterator> exception_type;
	EXPECT_THROW(parse(content.begin(), content.end(), syntax),
				 exception_type);
}

TEST_F(BadFrameMessageTest, ParseCharacterChannelNumber)
{
	static const std::string content =
		"MSG A 1 . 52 120\r\n"
		"Content-Type: application/beep+xml\r\n\r\n" // 38
		"<start number='1'>\r\n" // 20
		"   <profile uri='http://iana.org/beep/SASL/OTP' />\r\n" // 52
		"</start>\r\n" // 10
		"END\r\n";
	beep::frame_syntax syntax(frm, size);
	typedef parser_error<beep::frame_syntax_errors, std::string::const_iterator> exception_type;
	EXPECT_THROW(parse(content.begin(), content.end(), syntax),
				 exception_type);
}

TEST_F(BadFrameMessageTest, ParseStringChannelNumber)
{
	static const std::string content =
		"MSG BLAH 1 . 52 120\r\n"
		"Content-Type: application/beep+xml\r\n\r\n" // 38
		"<start number='1'>\r\n" // 20
		"   <profile uri='http://iana.org/beep/SASL/OTP' />\r\n" // 52
		"</start>\r\n" // 10
		"END\r\n";
	beep::frame_syntax syntax(frm, size);
	typedef parser_error<beep::frame_syntax_errors, std::string::const_iterator> exception_type;
	EXPECT_THROW(parse(content.begin(), content.end(), syntax),
				 exception_type);
}

TEST_F(BadFrameMessageTest, ParseNegativeChannelNumber)
{
	static const std::string content =
		"MSG -1 1 . 52 120\r\n"
		"Content-Type: application/beep+xml\r\n\r\n" // 38
		"<start number='1'>\r\n" // 20
		"   <profile uri='http://iana.org/beep/SASL/OTP' />\r\n" // 52
		"</start>\r\n" // 10
		"END\r\n";
	beep::frame_syntax syntax(frm, size);
	typedef parser_error<beep::frame_syntax_errors, std::string::const_iterator> exception_type;
	EXPECT_THROW(parse(content.begin(), content.end(), syntax),
				 exception_type);
}

TEST_F(BadFrameMessageTest, ParseHugeChannelNumber)
{
	static const std::string content =
		"MSG 2147483648 1 . 52 120\r\n"
		"Content-Type: application/beep+xml\r\n\r\n" // 38
		"<start number='1'>\r\n" // 20
		"   <profile uri='http://iana.org/beep/SASL/OTP' />\r\n" // 52
		"</start>\r\n" // 10
		"END\r\n";
	beep::frame_syntax syntax(frm, size);
	typedef parser_error<beep::frame_syntax_errors, std::string::const_iterator> exception_type;
	EXPECT_THROW(parse(content.begin(), content.end(), syntax),
				 exception_type);
}

TEST_F(BadFrameMessageTest, ParseChannelBoundaryNumber)
{
	static const std::string content =
		"MSG 2147483647 1 . 52 120\r\n"
		"Content-Type: application/beep+xml\r\n\r\n" // 38
		"<start number='1'>\r\n" // 20
		"   <profile uri='http://iana.org/beep/SASL/OTP' />\r\n" // 52
		"</start>\r\n" // 10
		"END\r\n";
	beep::frame_syntax syntax(frm, size);
	const parse_info<std::string::const_iterator> pi =
		parse(content.begin(), content.end(), syntax);
	EXPECT_TRUE(pi.hit);
	EXPECT_TRUE(pi.full);
	EXPECT_TRUE(pi.stop == content.end());
}

TEST_F(BadFrameMessageTest, ParseMessageNumber)
{
	//EXPECT_EQ(1U, frm.message());
}

TEST_F(BadFrameMessageTest, ParseMore)
{
	//EXPECT_EQ(false, frm.more());
}

TEST_F(BadFrameMessageTest, ParseSequenceNumber)
{
	//EXPECT_EQ(52U, frm.sequence());
}

TEST_F(BadFrameMessageTest, ParsePayload)
{
	const std::string payload =
		"Content-Type: application/beep+xml\r\n\r\n" // 38
			"<start number='1'>\r\n" // 20
			"   <profile uri='http://iana.org/beep/SASL/OTP' />\r\n" // 52
			"</start>\r\n" // 10
		;
	//EXPECT_EQ(payload, frm.payload());
}

///////////////////////////////////////////////////////////////////////////////
// Test valid "reply" frames (RPY)
///////////////////////////////////////////////////////////////////////////////
class FrameReplyTest : public testing::Test {
public:
	virtual ~FrameReplyTest() { }
protected:
	virtual void SetUp()
	{
		static const std::string content =
			"RPY 3 2 . 7 110\r\n"
			"Content-Type: application/beep+xml\r\n"
			"\r\n" 
			"<greeting>\r\n"
			"   <profile uri='http://iana.org/beep/TLS' />\r\n"
			"</greeting>\r\n"
			"END\r\n";
		size = 0;
		frm = beep::frame();
		beep::frame_syntax syntax(frm, size);
		const parse_info<std::string::const_iterator> pi =
			parse(content.begin(), content.end(), syntax);
		ASSERT_TRUE(pi.full);
	}

	virtual void TearDown()
	{
	}

	std::size_t size;
	beep::frame frm;
};

TEST_F(FrameReplyTest, ParseKeyword)
{
	EXPECT_EQ(beep::frame::rpy(), frm.header());
}

TEST_F(FrameReplyTest, ParseChannelNumber)
{
	EXPECT_EQ(3U, frm.channel());
}

TEST_F(FrameReplyTest, ParseMessageNumber)
{
	EXPECT_EQ(2U, frm.message());
}

TEST_F(FrameReplyTest, ParseMore)
{
	EXPECT_EQ(false, frm.more());
}

TEST_F(FrameReplyTest, ParseSequenceNumber)
{
	EXPECT_EQ(7U, frm.sequence());
}

TEST_F(FrameReplyTest, ParsePayload)
{
	const std::string payload =
		"Content-Type: application/beep+xml\r\n"
		"\r\n"
		"<greeting>\r\n"
		"   <profile uri='http://iana.org/beep/TLS' />\r\n"
		"</greeting>\r\n"
		;
	EXPECT_EQ(payload, frm.payload());
}

///////////////////////////////////////////////////////////////////////////////
// Test valid "answer" frames (ANS)
///////////////////////////////////////////////////////////////////////////////
class FrameAnswerTest : public testing::Test {
public:
	virtual ~FrameAnswerTest() { }
protected:
	virtual void SetUp()
	{
		static const std::string content =
			"ANS 1 0 . 40 10 1\r\n"
			"dan is 1\r\n"
			"END\r\n";
		size = 0;
		frm = beep::frame();
		beep::frame_syntax syntax(frm, size);
		const parse_info<std::string::const_iterator> pi =
			parse(content.begin(), content.end(), syntax);
		ASSERT_TRUE(pi.full);
	}

	virtual void TearDown()
	{
	}

	std::size_t size;
	beep::frame frm;
};

TEST_F(FrameAnswerTest, ParseKeyword)
{
	EXPECT_EQ(beep::frame::ans(), frm.header());
}

TEST_F(FrameAnswerTest, ParseChannelNumber)
{
	EXPECT_EQ(1U, frm.channel());
}

TEST_F(FrameAnswerTest, ParseMessageNumber)
{
	EXPECT_EQ(0U, frm.message());
}

TEST_F(FrameAnswerTest, ParseMore)
{
	EXPECT_EQ(false, frm.more());
}

TEST_F(FrameAnswerTest, ParseSequenceNumber)
{
	EXPECT_EQ(40U, frm.sequence());
}

TEST_F(FrameAnswerTest, ParsePayload)
{
	const std::string payload =
		"dan is 1\r\n"
		;
	EXPECT_EQ(payload, frm.payload());
}

TEST_F(FrameAnswerTest, ParseAnswerNumber)
{
	EXPECT_EQ(1u, frm.answer());
}

///////////////////////////////////////////////////////////////////////////////
// Test valid "error" frames (ERR)
///////////////////////////////////////////////////////////////////////////////
class FrameErrorTest : public testing::Test {
public:
	virtual ~FrameErrorTest() { }
protected:
	virtual void SetUp()
	{
		static const std::string content =
			"ERR 0 2 . 392 79\r\n"
			"Content-Type: application/beep+xml\r\n"
			"\r\n"
			"<error code='550'>still working</error>\r\n"
			"END\r\n";
		size = 0;
		frm = beep::frame();
		beep::frame_syntax syntax(frm, size);
		const parse_info<std::string::const_iterator> pi =
			parse(content.begin(), content.end(), syntax);
		ASSERT_TRUE(pi.full);
	}

	virtual void TearDown()
	{
	}

	std::size_t size;
	beep::frame frm;
};

TEST_F(FrameErrorTest, ParseKeyword)
{
	EXPECT_EQ(beep::frame::err(), frm.header());
}

TEST_F(FrameErrorTest, ParseChannelNumber)
{
	EXPECT_EQ(0U, frm.channel());
}

TEST_F(FrameErrorTest, ParseMessageNumber)
{
	EXPECT_EQ(2U, frm.message());
}

TEST_F(FrameErrorTest, ParseMore)
{
	EXPECT_EQ(false, frm.more());
}

TEST_F(FrameErrorTest, ParseSequenceNumber)
{
	EXPECT_EQ(392U, frm.sequence());
}

TEST_F(FrameErrorTest, ParsePayload)
{
	const std::string payload =
		"Content-Type: application/beep+xml\r\n"
		"\r\n"
		"<error code='550'>still working</error>\r\n"
		;
	EXPECT_EQ(payload, frm.payload());
}

///////////////////////////////////////////////////////////////////////////////
// Test valid "null" frames (NUL)
///////////////////////////////////////////////////////////////////////////////
class FrameNullTest : public testing::Test {
public:
	virtual ~FrameNullTest() { }
protected:
	virtual void SetUp()
	{
		static const std::string content =
			"NUL 0 2 . 392 0\r\n"
			"END\r\n";
		size = 0;
		frm = beep::frame();
		beep::frame_syntax syntax(frm, size);
		const parse_info<std::string::const_iterator> pi =
			parse(content.begin(), content.end(), syntax);
		ASSERT_TRUE(pi.full);
	}

	virtual void TearDown()
	{
	}

	std::size_t size;
	beep::frame frm;
};

TEST_F(FrameNullTest, ParseKeyword)
{
	EXPECT_EQ(beep::frame::nul(), frm.header());
}

TEST_F(FrameNullTest, ParseChannelNumber)
{
	EXPECT_EQ(0U, frm.channel());
}

TEST_F(FrameNullTest, ParseMessageNumber)
{
	EXPECT_EQ(2U, frm.message());
}

TEST_F(FrameNullTest, ParseMore)
{
	EXPECT_EQ(false, frm.more());
}

TEST_F(FrameNullTest, ParseSequenceNumber)
{
	EXPECT_EQ(392U, frm.sequence());
}

TEST_F(FrameNullTest, ParsePayload)
{
	const std::string payload;
	EXPECT_EQ(payload, frm.payload());
}

int
main(int argc, char **argv)
{
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
