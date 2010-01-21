/// \file test-frame.cpp
///
/// UNCLASSIFIED
//#define BOOST_SPIRIT_DEBUG
#include <gtest/gtest.h>

#include <string>
#include "beep/frame.hpp"
#include "beep/frame-stream.hpp"

typedef std::string::const_iterator iterator_type;
typedef beep::frame_parser<iterator_type> frame_parser;
using beep::frame;

namespace std {

std::ostream& operator<<(std::ostream &strm, const iterator_type &i)
{
	if (strm) {
		strm << "string iterator at " << *i;
	}
	return strm;
}

}

TEST(ChannelParser, Valid)
{
	static const std::string content = "MSG 10 2 . 3 0\r\nEND\r\n";

	frame_parser g; // the grammar
	std::string::const_iterator iter = content.begin();
	std::string::const_iterator end = content.end();
	frame myFrame;
	EXPECT_NO_THROW(EXPECT_TRUE(parse(iter, end, g, myFrame)));
	EXPECT_EQ(10u, myFrame.get_channel());
}

TEST(ChannelParser, Negative)
{
	static const std::string content = "MSG -10 2 . 3 0\r\nEND\r\n";

	frame_parser g; // the grammar
	std::string::const_iterator iter = content.begin();
	std::string::const_iterator end = content.end();
	frame myFrame;
	EXPECT_NO_THROW(EXPECT_FALSE(parse(iter, end, g, myFrame)));
	EXPECT_EQ(0, myFrame.get_channel());
}

TEST(ChannelParser, TooLarge)
{
	static const std::string content = "MSG 2147483648 2 . 3 0\r\nEND\r\n";

	frame_parser g; // the grammar
	std::string::const_iterator iter = content.begin();
	std::string::const_iterator end = content.end();
	frame myFrame;
	EXPECT_NO_THROW(EXPECT_FALSE(parse(iter, end, g, myFrame)));
	EXPECT_EQ(2147483648u, myFrame.get_channel());
}

TEST(MessageNumberParser, Valid)
{
	static const std::string content = "MSG 19 3 . 3 0\r\nEND\r\n";

	frame_parser g; // the grammar
	std::string::const_iterator iter = content.begin();
	std::string::const_iterator end = content.end();
	frame myFrame;
	EXPECT_NO_THROW(EXPECT_TRUE(parse(iter, end, g, myFrame)));
	EXPECT_EQ(3u, myFrame.get_message());
}

TEST(SequenceParser, Valid)
{
	static const std::string content = "MSG 19 3 . 6 0\r\nEND\r\n";

	frame_parser g; // the grammar
	std::string::const_iterator iter = content.begin();
	std::string::const_iterator end = content.end();
	frame myFrame;
	EXPECT_NO_THROW(EXPECT_TRUE(parse(iter, end, g, myFrame)));
	EXPECT_EQ(6u, myFrame.get_sequence());
}

TEST(SizeParser, Valid)
{
	static const std::string content = "MSG 19 3 . 6 3\r\nABCEND\r\n";

	frame_parser g; // the grammar
	std::string::const_iterator iter = content.begin();
	std::string::const_iterator end = content.end();
	frame myFrame;
	EXPECT_NO_THROW(EXPECT_TRUE(parse(iter, end, g, myFrame)));
	EXPECT_EQ(3u, myFrame.get_payload().size());
	EXPECT_EQ("ABC", myFrame.get_payload());
}

TEST(AnswerParser, Valid)
{
	static const std::string content = "ANS 19 3 . 6 3 19\r\nABCEND\r\n";

	frame_parser g; // the grammar
	std::string::const_iterator iter = content.begin();
	std::string::const_iterator end = content.end();
	frame myFrame;
	EXPECT_NO_THROW(EXPECT_TRUE(parse(iter, end, g, myFrame)));
	EXPECT_EQ(beep::ANS, myFrame.get_type());
	EXPECT_EQ(19u, myFrame.get_answer());
}

TEST(MessageFrameHeader, Valid)
{
	static const std::string content = "MSG 19 2 . 3 0\r\nEND\r\n";

	frame_parser g; // the grammar
	std::string::const_iterator iter = content.begin();
	std::string::const_iterator end = content.end();
	beep::frame aFrame;
	EXPECT_NO_THROW(parse(iter, end, g, aFrame));
	EXPECT_EQ(beep::MSG, aFrame.get_type());
	EXPECT_EQ(19u, aFrame.get_channel());
	EXPECT_EQ(2u, aFrame.get_message());
	EXPECT_FALSE(aFrame.get_more());
	EXPECT_EQ(3u, aFrame.get_sequence());
}

TEST(ReplyHeader, Valid)
{
	static const std::string content = "RPY 19 2 . 3 0\r\nEND\r\n";

	frame_parser g; // the grammar
	std::string::const_iterator iter = content.begin();
	std::string::const_iterator end = content.end();
	beep::frame aFrame;
	EXPECT_NO_THROW(parse(iter, end, g, aFrame));
	EXPECT_EQ(beep::RPY, aFrame.get_type());
	EXPECT_EQ(19u, aFrame.get_channel());
	EXPECT_EQ(2u, aFrame.get_message());
	EXPECT_FALSE(aFrame.get_more());
	EXPECT_EQ(3u, aFrame.get_sequence());
}

TEST(AnswerHeader, Valid)
{
	static const std::string content = "ANS 19 2 . 3 0 5\r\nEND\r\n";

	frame_parser g; // the grammar
	std::string::const_iterator iter = content.begin();
	std::string::const_iterator end = content.end();
	beep::frame aFrame;
	EXPECT_NO_THROW(parse(iter, end, g, aFrame));
	EXPECT_EQ(beep::ANS, aFrame.get_type());
	EXPECT_EQ(19u, aFrame.get_channel());
	EXPECT_EQ(2u, aFrame.get_message());
	EXPECT_FALSE(aFrame.get_more());
	EXPECT_EQ(3u, aFrame.get_sequence());
	EXPECT_EQ(5u, aFrame.get_answer());
}

TEST(ErrorHeader, Valid)
{
	static const std::string content = "ERR 19 2 . 3 0\r\nEND\r\n";

	frame_parser g; // the grammar
	std::string::const_iterator iter = content.begin();
	std::string::const_iterator end = content.end();
	beep::frame aFrame;
	EXPECT_NO_THROW(parse(iter, end, g, aFrame));
	EXPECT_EQ(beep::ERR, aFrame.get_type());
	EXPECT_EQ(19u, aFrame.get_channel());
	EXPECT_EQ(2u, aFrame.get_message());
	EXPECT_FALSE(aFrame.get_more());
	EXPECT_EQ(3u, aFrame.get_sequence());
}

TEST(NullHeader, Valid)
{
	static const std::string content = "NUL 19 2 . 3 0\r\nEND\r\n";

	frame_parser g; // the grammar
	std::string::const_iterator iter = content.begin();
	std::string::const_iterator end = content.end();
	beep::frame aFrame;
	EXPECT_NO_THROW(parse(iter, end, g, aFrame));
	EXPECT_EQ(beep::NUL, aFrame.get_type());
	EXPECT_EQ(19u, aFrame.get_channel());
	EXPECT_EQ(2u, aFrame.get_message());
	EXPECT_FALSE(aFrame.get_more());
	EXPECT_EQ(3u, aFrame.get_sequence());
}

TEST(PayloadParse, Valid)
{
	static const std::string content = "MSG 19 2 . 3 12\r\nSome PayloadEND\r\n";

	frame_parser g; // the grammar
	std::string::const_iterator iter = content.begin();
	std::string::const_iterator end = content.end();
	beep::frame aFrame;

	EXPECT_NO_THROW(parse(iter, end, g, aFrame));
	EXPECT_EQ(beep::MSG, aFrame.get_type());
	EXPECT_EQ(19u, aFrame.get_channel());
	EXPECT_EQ(2u, aFrame.get_message());
	EXPECT_FALSE(aFrame.get_more());
	EXPECT_EQ(3u, aFrame.get_sequence());
	EXPECT_EQ("Some Payload", aFrame.get_payload());
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
		frm = beep::frame();
		frame_parser syntax;
		std::string::const_iterator iter = content.begin();
		std::string::const_iterator end = content.end();
		EXPECT_NO_THROW(EXPECT_TRUE(parse(iter, end, syntax, frm)));
		EXPECT_EQ(iter, end);
	}

	virtual void TearDown()
	{
	}

	beep::frame frm;
};

TEST_F(FrameMessageTest, ParseKeyword)
{
	EXPECT_EQ(beep::MSG, frm.get_type());
}

TEST_F(FrameMessageTest, ParseChannelNumber)
{
	EXPECT_EQ(9U, frm.get_channel());
}

TEST_F(FrameMessageTest, ParseMessageNumber)
{
	EXPECT_EQ(1U, frm.get_message());
}

TEST_F(FrameMessageTest, ParseMore)
{
	EXPECT_EQ(false, frm.get_more());
}

TEST_F(FrameMessageTest, ParseSequenceNumber)
{
	EXPECT_EQ(52U, frm.get_sequence());
}

TEST_F(FrameMessageTest, ParsePayload)
{
	const std::string payload =
		"Content-Type: application/beep+xml\r\n\r\n" // 38
			"<start number='1'>\r\n" // 20
			"   <profile uri='http://iana.org/beep/SASL/OTP' />\r\n" // 52
			"</start>\r\n" // 10
		;
	EXPECT_EQ(payload, frm.get_payload());
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
		frm = beep::frame();
	}

	virtual void TearDown()
	{
	}
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
	frame_parser syntax;
	std::string::const_iterator iter = content.begin();
	std::string::const_iterator end = content.end();
	EXPECT_FALSE(parse(iter, end, syntax, frm));
	EXPECT_EQ(content.begin(), iter);
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
	frame_parser syntax;
	std::string::const_iterator iter = content.begin();
	std::string::const_iterator end = content.end();
	EXPECT_FALSE(parse(iter, end, syntax, frm));
	EXPECT_EQ(content.begin(), iter);
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
	frame_parser syntax;
	std::string::const_iterator iter = content.begin();
	std::string::const_iterator end = content.end();
	EXPECT_FALSE(parse(iter, end, syntax, frm));
	EXPECT_EQ(content.begin(), iter);
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
	frame_parser syntax;
	std::string::const_iterator iter = content.begin();
	std::string::const_iterator end = content.end();
	EXPECT_FALSE(parse(iter, end, syntax, frm));
	EXPECT_EQ(content.begin(), iter);
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
	frame_parser syntax;
	std::string::const_iterator iter = content.begin();
	std::string::const_iterator end = content.end();
	EXPECT_FALSE(parse(iter, end, syntax, frm));
	EXPECT_EQ(content.begin(), iter);
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
	frame_parser syntax;
	std::string::const_iterator iter = content.begin();
	std::string::const_iterator end = content.end();
	EXPECT_TRUE(parse(iter, end, syntax, frm));
	EXPECT_EQ(end, iter);
	EXPECT_EQ(2147483647, frm.get_channel());
}

TEST_F(BadFrameMessageTest, ParseMessageNumber)

{	static const std::string content =
		"MSG 2147483647 1 . 52 120\r\n"
		"Content-Type: application/beep+xml\r\n\r\n" // 38
		"<start number='1'>\r\n" // 20
		"   <profile uri='http://iana.org/beep/SASL/OTP' />\r\n" // 52
		"</start>\r\n" // 10
		"END\r\n";
	frame_parser syntax;
	std::string::const_iterator iter = content.begin();
	std::string::const_iterator end = content.end();
	EXPECT_TRUE(parse(iter, end, syntax, frm));
	EXPECT_EQ(end, iter);
	EXPECT_EQ(1U, frm.get_message());
}

TEST_F(BadFrameMessageTest, ParseMore)
{
	static const std::string content =
		"MSG 2147483647 1 . 52 120\r\n"
		"Content-Type: application/beep+xml\r\n\r\n" // 38
		"<start number='1'>\r\n" // 20
		"   <profile uri='http://iana.org/beep/SASL/OTP' />\r\n" // 52
		"</start>\r\n" // 10
		"END\r\n";
	frame_parser syntax;
	std::string::const_iterator iter = content.begin();
	std::string::const_iterator end = content.end();
	EXPECT_TRUE(parse(iter, end, syntax, frm));
	EXPECT_EQ(end, iter);
	EXPECT_EQ(false, frm.get_more());
}

TEST_F(BadFrameMessageTest, ParseSequenceNumber)
{
	static const std::string content =
		"MSG 2147483647 1 . 52 120\r\n"
		"Content-Type: application/beep+xml\r\n\r\n" // 38
		"<start number='1'>\r\n" // 20
		"   <profile uri='http://iana.org/beep/SASL/OTP' />\r\n" // 52
		"</start>\r\n" // 10
		"END\r\n";
	frame_parser syntax;
	std::string::const_iterator iter = content.begin();
	std::string::const_iterator end = content.end();
	EXPECT_TRUE(parse(iter, end, syntax, frm));
	EXPECT_EQ(end, iter);
	EXPECT_EQ(52U, frm.get_sequence());
}

TEST_F(BadFrameMessageTest, ParsePayload)
{
	static const std::string content =
		"MSG 2147483647 1 . 52 120\r\n"
		"Content-Type: application/beep+xml\r\n\r\n" // 38
		"<start number='1'>\r\n" // 20
		"   <profile uri='http://iana.org/beep/SASL/OTP' />\r\n" // 52
		"</start>\r\n" // 10
		"END\r\n";
	frame_parser syntax;
	std::string::const_iterator iter = content.begin();
	std::string::const_iterator end = content.end();
	EXPECT_TRUE(parse(iter, end, syntax, frm));
	EXPECT_EQ(end, iter);
	const std::string payload =
		"Content-Type: application/beep+xml\r\n\r\n" // 38
			"<start number='1'>\r\n" // 20
			"   <profile uri='http://iana.org/beep/SASL/OTP' />\r\n" // 52
			"</start>\r\n" // 10
		;
	EXPECT_EQ(payload, frm.get_payload());
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
		frm = beep::frame();

		frame_parser syntax;
		std::string::const_iterator iter = content.begin();
		std::string::const_iterator end = content.end();
		EXPECT_TRUE(parse(iter, end, syntax, frm));
		EXPECT_EQ(end, iter);
	}

	virtual void TearDown()
	{
	}

	beep::frame frm;
};

TEST_F(FrameReplyTest, ParseKeyword)
{
	EXPECT_EQ(beep::RPY, frm.get_type());
}

TEST_F(FrameReplyTest, ParseChannelNumber)
{
	EXPECT_EQ(3U, frm.get_channel());
}

TEST_F(FrameReplyTest, ParseMessageNumber)
{
	EXPECT_EQ(2U, frm.get_message());
}

TEST_F(FrameReplyTest, ParseMore)
{
	EXPECT_EQ(false, frm.get_more());
}

TEST_F(FrameReplyTest, ParseSequenceNumber)
{
	EXPECT_EQ(7U, frm.get_sequence());
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
	EXPECT_EQ(payload, frm.get_payload());
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
		frm = beep::frame();

		frame_parser syntax;
		std::string::const_iterator iter = content.begin();
		std::string::const_iterator end = content.end();
		EXPECT_TRUE(parse(iter, end, syntax, frm));
		EXPECT_EQ(end, iter);
	}

	virtual void TearDown()
	{
	}

	beep::frame frm;
};

TEST_F(FrameAnswerTest, ParseKeyword)
{
	EXPECT_EQ(beep::ANS, frm.get_type());
}

TEST_F(FrameAnswerTest, ParseChannelNumber)
{
	EXPECT_EQ(1U, frm.get_channel());
}

TEST_F(FrameAnswerTest, ParseMessageNumber)
{
	EXPECT_EQ(0U, frm.get_message());
}

TEST_F(FrameAnswerTest, ParseMore)
{
	EXPECT_EQ(false, frm.get_more());
}

TEST_F(FrameAnswerTest, ParseSequenceNumber)
{
	EXPECT_EQ(40U, frm.get_sequence());
}

TEST_F(FrameAnswerTest, ParsePayload)
{
	const std::string payload =
		"dan is 1\r\n"
		;
	EXPECT_EQ(payload, frm.get_payload());
}

TEST_F(FrameAnswerTest, ParseAnswerNumber)
{
	EXPECT_EQ(1u, frm.get_answer());
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
		frm = beep::frame();

		frame_parser syntax;
		std::string::const_iterator iter = content.begin();
		std::string::const_iterator end = content.end();
		EXPECT_TRUE(parse(iter, end, syntax, frm));
		EXPECT_EQ(end, iter);
	}

	virtual void TearDown()
	{
	}

	beep::frame frm;
};

TEST_F(FrameErrorTest, ParseKeyword)
{
	EXPECT_EQ(beep::ERR, frm.get_type());
}

TEST_F(FrameErrorTest, ParseChannelNumber)
{
	EXPECT_EQ(0U, frm.get_channel());
}

TEST_F(FrameErrorTest, ParseMessageNumber)
{
	EXPECT_EQ(2U, frm.get_message());
}

TEST_F(FrameErrorTest, ParseMore)
{
	EXPECT_EQ(false, frm.get_more());
}

TEST_F(FrameErrorTest, ParseSequenceNumber)
{
	EXPECT_EQ(392U, frm.get_sequence());
}

TEST_F(FrameErrorTest, ParsePayload)
{
	const std::string payload =
		"Content-Type: application/beep+xml\r\n"
		"\r\n"
		"<error code='550'>still working</error>\r\n"
		;
	EXPECT_EQ(payload, frm.get_payload());
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
		frm = beep::frame();

		frame_parser syntax;
		std::string::const_iterator iter = content.begin();
		std::string::const_iterator end = content.end();
		EXPECT_TRUE(parse(iter, end, syntax, frm));
		EXPECT_EQ(end, iter);
	}

	virtual void TearDown()
	{
	}

	beep::frame frm;
};

TEST_F(FrameNullTest, ParseKeyword)
{
	EXPECT_EQ(beep::NUL, frm.get_type());
}

TEST_F(FrameNullTest, ParseChannelNumber)
{
	EXPECT_EQ(0U, frm.get_channel());
}

TEST_F(FrameNullTest, ParseMessageNumber)
{
	EXPECT_EQ(2U, frm.get_message());
}

TEST_F(FrameNullTest, ParseMore)
{
	EXPECT_EQ(false, frm.get_more());
}

TEST_F(FrameNullTest, ParseSequenceNumber)
{
	EXPECT_EQ(392U, frm.get_sequence());
}

TEST_F(FrameNullTest, ParsePayload)
{
	const std::string payload;
	EXPECT_EQ(payload, frm.get_payload());
}

int
main(int argc, char **argv)
{
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
