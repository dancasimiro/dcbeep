/// \file test-frame.cpp
///
/// UNCLASSIFIED
//#define BOOST_SPIRIT_DEBUG
#include <gtest/gtest.h>

#include <string>
#include "beep/frame.hpp"
#include "beep/frame-stream.hpp"

using beep::frame;
using boost::get;

TEST(ChannelParser, Valid)
{
	static const std::string content = "MSG 10 2 . 3 0\r\nEND\r\n";

	frame myFrame;
	EXPECT_NO_THROW(myFrame = beep::parse_frame(content));
	EXPECT_EQ(10u, get<beep::msg_frame>(myFrame).channel);
}

TEST(ChannelParser, Negative)
{
	static const std::string content = "MSG -10 2 . 3 0\r\nEND\r\n";

	frame myFrame;
	EXPECT_ANY_THROW(myFrame = beep::parse_frame(content));
	EXPECT_EQ(0, get<beep::msg_frame>(myFrame).channel);
}

TEST(ChannelParser, TooLarge)
{
	static const std::string content = "MSG 2147483648 2 . 3 0\r\nEND\r\n";

	frame myFrame;
	EXPECT_ANY_THROW(myFrame = beep::parse_frame(content));
	EXPECT_EQ(0, get<beep::msg_frame>(myFrame).channel);
}

TEST(MessageNumberParser, Valid)
{
	static const std::string content = "MSG 19 3 . 3 0\r\nEND\r\n";

	frame myFrame;
	EXPECT_NO_THROW(myFrame = beep::parse_frame(content));
	EXPECT_EQ(3u, get<beep::msg_frame>(myFrame).message);
}

TEST(SequenceParser, Valid)
{
	static const std::string content = "MSG 19 3 . 6 0\r\nEND\r\n";

	frame myFrame;
	EXPECT_NO_THROW(myFrame = beep::parse_frame(content));
	EXPECT_EQ(6u, get<beep::msg_frame>(myFrame).sequence);
}

TEST(SizeParser, Valid)
{
	static const std::string content = "MSG 19 3 . 6 3\r\nABCEND\r\n";

	frame myFrame;
	EXPECT_NO_THROW(myFrame = beep::parse_frame(content));
	EXPECT_EQ(3u, get<beep::msg_frame>(myFrame).payload.size());
	EXPECT_EQ("ABC", get<beep::msg_frame>(myFrame).payload);
}

TEST(AnswerParser, Valid)
{
	static const std::string content = "ANS 19 3 . 6 3 19\r\nABCEND\r\n";

	frame myFrame;
	EXPECT_NO_THROW(myFrame = beep::parse_frame(content));
	EXPECT_NO_THROW(get<beep::ans_frame>(myFrame));
	EXPECT_EQ(19u, get<beep::ans_frame>(myFrame).answer);
}

TEST(MessageFrameHeader, Valid)
{
	static const std::string content = "MSG 19 2 . 3 0\r\nEND\r\n";

	beep::frame aFrame;
	EXPECT_NO_THROW(aFrame = beep::parse_frame(content));
	EXPECT_NO_THROW(get<beep::msg_frame>(aFrame));
	EXPECT_EQ(19u, get<beep::msg_frame>(aFrame).channel);
	EXPECT_EQ(2u, get<beep::msg_frame>(aFrame).message);
	EXPECT_FALSE(get<beep::msg_frame>(aFrame).more);
	EXPECT_EQ(3u, get<beep::msg_frame>(aFrame).sequence);
}

TEST(ReplyHeader, Valid)
{
	static const std::string content = "RPY 19 2 . 3 0\r\nEND\r\n";

	beep::frame aFrame;
	EXPECT_NO_THROW(aFrame = beep::parse_frame(content));
	EXPECT_NO_THROW(get<beep::rpy_frame>(aFrame));
	EXPECT_EQ(19u, get<beep::rpy_frame>(aFrame).channel);
	EXPECT_EQ(2u, get<beep::rpy_frame>(aFrame).message);
	EXPECT_FALSE(get<beep::rpy_frame>(aFrame).more);
	EXPECT_EQ(3u, get<beep::rpy_frame>(aFrame).sequence);
}

TEST(AnswerHeader, Valid)
{
	static const std::string content = "ANS 19 2 . 3 0 5\r\nEND\r\n";

	beep::frame aFrame;
	EXPECT_NO_THROW(aFrame = beep::parse_frame(content));
	EXPECT_NO_THROW(get<beep::ans_frame>(aFrame));
	EXPECT_EQ(19u, get<beep::ans_frame>(aFrame).channel);
	EXPECT_EQ(2u, get<beep::ans_frame>(aFrame).message);
	EXPECT_FALSE(get<beep::ans_frame>(aFrame).more);
	EXPECT_EQ(3u, get<beep::ans_frame>(aFrame).sequence);
	EXPECT_EQ(5u, get<beep::ans_frame>(aFrame).answer);
}

TEST(ErrorHeader, Valid)
{
	static const std::string content = "ERR 19 2 . 3 0\r\nEND\r\n";

	beep::frame aFrame;
	EXPECT_NO_THROW(aFrame = beep::parse_frame(content));
	EXPECT_NO_THROW(get<beep::err_frame>(aFrame));
	EXPECT_EQ(19u, get<beep::err_frame>(aFrame).channel);
	EXPECT_EQ(2u, get<beep::err_frame>(aFrame).message);
	EXPECT_FALSE(get<beep::err_frame>(aFrame).more);
	EXPECT_EQ(3u, get<beep::err_frame>(aFrame).sequence);
}

TEST(NullHeader, Valid)
{
	static const std::string content = "NUL 19 2 . 3 0\r\nEND\r\n";

	beep::frame aFrame;
	EXPECT_NO_THROW(aFrame = beep::parse_frame(content));
	EXPECT_NO_THROW(get<beep::nul_frame>(aFrame));
	EXPECT_EQ(19u, get<beep::nul_frame>(aFrame).channel);
	EXPECT_EQ(2u, get<beep::nul_frame>(aFrame).message);
	EXPECT_FALSE(get<beep::nul_frame>(aFrame).more);
	EXPECT_EQ(3u, get<beep::nul_frame>(aFrame).sequence);
}

TEST(SeqHeader, Valid)
{
	static const std::string content = "SEQ 3 2 4096\r\n";

	beep::frame aFrame;
	EXPECT_NO_THROW(aFrame = beep::parse_frame(content));
	EXPECT_NO_THROW(get<beep::seq_frame>(aFrame));
	EXPECT_EQ(3u, get<beep::seq_frame>(aFrame).channel);
	EXPECT_EQ(2u, get<beep::seq_frame>(aFrame).acknowledgement);
	EXPECT_EQ(4096u, get<beep::seq_frame>(aFrame).window);
}

TEST(PayloadParse, Valid)
{
	static const std::string content = "MSG 19 2 . 3 12\r\nSome PayloadEND\r\n";

	beep::frame aFrame;
	EXPECT_NO_THROW(aFrame = beep::parse_frame(content));
	EXPECT_NO_THROW(get<beep::msg_frame>(aFrame));
	EXPECT_EQ(19u, get<beep::msg_frame>(aFrame).channel);
	EXPECT_EQ(2u, get<beep::msg_frame>(aFrame).message);
	EXPECT_FALSE(get<beep::msg_frame>(aFrame).more);
	EXPECT_EQ(3u, get<beep::msg_frame>(aFrame).sequence);
	EXPECT_EQ("Some Payload", get<beep::msg_frame>(aFrame).payload);
}

TEST(BinaryParse, Valid)
{
	const float some_data[] = { 1.0f, 2.0f, 3.0f, 4.0f };
	const std::string base = "MSG 19 2 . 3 ";
	std::ostringstream strm;
	strm << base << sizeof(some_data) << "\r\n";
	strm.write(reinterpret_cast<const char*>(some_data), sizeof(some_data));
	strm << "END\r\n";
	const std::string content = strm.str();
	EXPECT_EQ(content.length(), 17 + 5 + sizeof(float) * 4);

	beep::frame aFrame;
	EXPECT_NO_THROW(aFrame = beep::parse_frame(content));
	EXPECT_NO_THROW(get<beep::msg_frame>(aFrame));
	EXPECT_EQ(19u, get<beep::msg_frame>(aFrame).channel);
	EXPECT_EQ(2u, get<beep::msg_frame>(aFrame).message);
	EXPECT_FALSE(get<beep::msg_frame>(aFrame).more);
	EXPECT_EQ(3u, get<beep::msg_frame>(aFrame).sequence);
	EXPECT_EQ(sizeof(some_data), get<beep::msg_frame>(aFrame).payload.length());

	float out_data[4] = { 0 };
	std::istringstream payload_strm(get<beep::msg_frame>(aFrame).payload);
	payload_strm.read(reinterpret_cast<char*>(out_data), sizeof(out_data));
	EXPECT_FLOAT_EQ(1.0f, out_data[0]);
	EXPECT_FLOAT_EQ(2.0f, out_data[1]);
	EXPECT_FLOAT_EQ(3.0f, out_data[2]);
	EXPECT_FLOAT_EQ(4.0f, out_data[3]);
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
		EXPECT_NO_THROW(frm = beep::parse_frame(content));
	}

	virtual void TearDown()
	{
	}

	beep::frame frm;
};

TEST_F(FrameMessageTest, ParseKeyword)
{
	EXPECT_NO_THROW(get<beep::msg_frame>(frm));
	EXPECT_EQ(beep::MSG, get<beep::msg_frame>(frm).header());
}

TEST_F(FrameMessageTest, ParseChannelNumber)
{
	EXPECT_EQ(9U, get<beep::msg_frame>(frm).channel);
}

TEST_F(FrameMessageTest, ParseMessageNumber)
{
	EXPECT_EQ(1U, get<beep::msg_frame>(frm).message);
}

TEST_F(FrameMessageTest, ParseMore)
{
	EXPECT_EQ(false, get<beep::msg_frame>(frm).more);
}

TEST_F(FrameMessageTest, ParseSequenceNumber)
{
	EXPECT_EQ(52U, get<beep::msg_frame>(frm).sequence);
}

TEST_F(FrameMessageTest, ParsePayload)
{
	const std::string payload =
		"Content-Type: application/beep+xml\r\n\r\n" // 38
			"<start number='1'>\r\n" // 20
			"   <profile uri='http://iana.org/beep/SASL/OTP' />\r\n" // 52
			"</start>\r\n" // 10
		;
	EXPECT_EQ(payload, get<beep::msg_frame>(frm).payload);
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
	EXPECT_ANY_THROW(beep::parse_frame(content));
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
	EXPECT_ANY_THROW(beep::parse_frame(content));
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
	EXPECT_ANY_THROW(beep::parse_frame(content));
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
	EXPECT_ANY_THROW(beep::parse_frame(content));
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
	EXPECT_ANY_THROW(beep::parse_frame(content));
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
	EXPECT_NO_THROW(frm = beep::parse_frame(content));
	EXPECT_EQ(2147483647, get<beep::msg_frame>(frm).channel);
}

TEST_F(BadFrameMessageTest, ParseMessageNumber)

{	static const std::string content =
		"MSG 2147483647 1 . 52 120\r\n"
		"Content-Type: application/beep+xml\r\n\r\n" // 38
		"<start number='1'>\r\n" // 20
		"   <profile uri='http://iana.org/beep/SASL/OTP' />\r\n" // 52
		"</start>\r\n" // 10
		"END\r\n";
	EXPECT_NO_THROW(frm = beep::parse_frame(content));
	EXPECT_EQ(1U, get<beep::msg_frame>(frm).message);
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
	EXPECT_NO_THROW(frm = beep::parse_frame(content));
	EXPECT_EQ(false, get<beep::msg_frame>(frm).more);
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
	EXPECT_NO_THROW(frm = beep::parse_frame(content));
	EXPECT_EQ(52U, get<beep::msg_frame>(frm).sequence);
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
	EXPECT_NO_THROW(frm = beep::parse_frame(content));
	const std::string payload =
		"Content-Type: application/beep+xml\r\n\r\n" // 38
			"<start number='1'>\r\n" // 20
			"   <profile uri='http://iana.org/beep/SASL/OTP' />\r\n" // 52
			"</start>\r\n" // 10
		;
	EXPECT_EQ(payload, get<beep::msg_frame>(frm).payload);
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
		EXPECT_NO_THROW(frm = beep::parse_frame(content));
	}

	virtual void TearDown()
	{
	}

	beep::frame frm;
};

TEST_F(FrameReplyTest, ParseKeyword)
{
	EXPECT_NO_THROW(get<beep::rpy_frame>(frm));
	EXPECT_EQ(beep::RPY, get<beep::rpy_frame>(frm).header());
}

TEST_F(FrameReplyTest, ParseChannelNumber)
{
	EXPECT_EQ(3U, get<beep::rpy_frame>(frm).channel);
}

TEST_F(FrameReplyTest, ParseMessageNumber)
{
	EXPECT_EQ(2U, get<beep::rpy_frame>(frm).message);
}

TEST_F(FrameReplyTest, ParseMore)
{
	EXPECT_EQ(false, get<beep::rpy_frame>(frm).more);
}

TEST_F(FrameReplyTest, ParseSequenceNumber)
{
	EXPECT_EQ(7U, get<beep::rpy_frame>(frm).sequence);
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
	EXPECT_EQ(payload, get<beep::rpy_frame>(frm).payload);
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
		EXPECT_NO_THROW(frm = beep::parse_frame(content));
	}

	virtual void TearDown()
	{
	}

	beep::frame frm;
};

TEST_F(FrameAnswerTest, ParseKeyword)
{
	EXPECT_NO_THROW(get<beep::ans_frame>(frm).header());
	EXPECT_EQ(beep::ANS, get<beep::ans_frame>(frm).header());
}

TEST_F(FrameAnswerTest, ParseChannelNumber)
{
	EXPECT_EQ(1U, get<beep::ans_frame>(frm).channel);
}

TEST_F(FrameAnswerTest, ParseMessageNumber)
{
	EXPECT_EQ(0U, get<beep::ans_frame>(frm).message);
}

TEST_F(FrameAnswerTest, ParseMore)
{
	EXPECT_EQ(false, get<beep::ans_frame>(frm).more);
}

TEST_F(FrameAnswerTest, ParseSequenceNumber)
{
	EXPECT_EQ(40U, get<beep::ans_frame>(frm).sequence);
}

TEST_F(FrameAnswerTest, ParsePayload)
{
	const std::string payload =
		"dan is 1\r\n"
		;
	EXPECT_EQ(payload, get<beep::ans_frame>(frm).payload);
}

TEST_F(FrameAnswerTest, ParseAnswerNumber)
{
	EXPECT_EQ(1u, get<beep::ans_frame>(frm).answer);
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
		EXPECT_NO_THROW(frm = beep::parse_frame(content));
	}

	virtual void TearDown()
	{
	}

	beep::frame frm;
};

TEST_F(FrameErrorTest, ParseKeyword)
{
	EXPECT_NO_THROW(EXPECT_EQ(beep::ERR, get<beep::err_frame>(frm).header()));
}

TEST_F(FrameErrorTest, ParseChannelNumber)
{
	EXPECT_EQ(0U, get<beep::err_frame>(frm).channel);
}

TEST_F(FrameErrorTest, ParseMessageNumber)
{
	EXPECT_EQ(2U, get<beep::err_frame>(frm).message);
}

TEST_F(FrameErrorTest, ParseMore)
{
	EXPECT_EQ(false, get<beep::err_frame>(frm).more);
}

TEST_F(FrameErrorTest, ParseSequenceNumber)
{
	EXPECT_EQ(392U, get<beep::err_frame>(frm).sequence);
}

TEST_F(FrameErrorTest, ParsePayload)
{
	const std::string payload =
		"Content-Type: application/beep+xml\r\n"
		"\r\n"
		"<error code='550'>still working</error>\r\n"
		;
	EXPECT_EQ(payload, get<beep::err_frame>(frm).payload);
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
		EXPECT_NO_THROW(frm = beep::parse_frame(content));
	}

	virtual void TearDown()
	{
	}

	beep::frame frm;
};

TEST_F(FrameNullTest, ParseKeyword)
{
	EXPECT_NO_THROW(EXPECT_EQ(beep::NUL, get<beep::nul_frame>(frm).header()));
}

TEST_F(FrameNullTest, ParseChannelNumber)
{
	EXPECT_EQ(0U, get<beep::nul_frame>(frm).channel);
}

TEST_F(FrameNullTest, ParseMessageNumber)
{
	EXPECT_EQ(2U, get<beep::nul_frame>(frm).message);
}

TEST_F(FrameNullTest, ParseMore)
{
	EXPECT_EQ(false, get<beep::nul_frame>(frm).more);
}

TEST_F(FrameNullTest, ParseSequenceNumber)
{
	EXPECT_EQ(392U, get<beep::nul_frame>(frm).sequence);
}

TEST_F(FrameNullTest, ParsePayload)
{
	const std::string payload;
	EXPECT_EQ(payload, get<beep::nul_frame>(frm).payload);
}

int
main(int argc, char **argv)
{
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
