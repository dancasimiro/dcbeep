#include <gtest/gtest.h>

#include <string>
#include "beep/frame.hpp"

///////////////////////////////////////////////////////////////////////////////
// Test valid "message" frames (MSG)
///////////////////////////////////////////////////////////////////////////////
class FrameMessageTest : public testing::Test {
public:
	~FrameMessageTest() { }
protected:
	virtual void SetUp()
	{
		static const std::string frame_content =
			"MSG 9 1 . 52 120\r\n"
			"Content-Type: application/beep+xml\r\n\r\n" // 38
			"<start number='1'>\r\n" // 20
			"   <profile uri='http://iana.org/beep/SASL/OTP' />\r\n" // 52
			"</start>\r\n" // 10
			"END\r\n";
		beep::frame_parser frame_p;
		ASSERT_NO_THROW(frame_p(frame_content, frm));
	}

	virtual void TearDown()
	{
	}

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
// Test valid "reply" frames (RPY)
///////////////////////////////////////////////////////////////////////////////
class FrameReplyTest : public testing::Test {
public:
	~FrameReplyTest() { }
protected:
	virtual void SetUp()
	{
		static const std::string frame_content =
			"RPY 3 2 . 7 110\r\n"
			"Content-Type: application/beep+xml\r\n"
			"\r\n" 
			"<greeting>\r\n"
			"   <profile uri='http://iana.org/beep/TLS' />\r\n"
			"</greeting>\r\n"
			"END\r\n";
		beep::frame_parser frame_p;
		ASSERT_NO_THROW(frame_p(frame_content, frm));
	}

	virtual void TearDown()
	{
	}

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
	~FrameAnswerTest() { }
protected:
	virtual void SetUp()
	{
		static const std::string frame_content =
			"ANS 1 0 . 40 10 1\r\n"
			"dan is 1\r\n"
			"END\r\n";
		beep::frame_parser frame_p;
		ASSERT_NO_THROW(frame_p(frame_content, frm));
	}

	virtual void TearDown()
	{
	}

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
	~FrameErrorTest() { }
protected:
	virtual void SetUp()
	{
		static const std::string frame_content =
			"ERR 0 2 . 392 79\r\n"
			"Content-Type: application/beep+xml\r\n"
			"\r\n"
			"<error code='550'>still working</error>\r\n"
			"END\r\n";
		beep::frame_parser frame_p;
		ASSERT_NO_THROW(frame_p(frame_content, frm));
	}

	virtual void TearDown()
	{
	}

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
	~FrameNullTest() { }
protected:
	virtual void SetUp()
	{
		static const std::string frame_content =
			"NUL 0 2 . 392 0\r\n"
			"END\r\n";
		beep::frame_parser frame_p;
		ASSERT_NO_THROW(frame_p(frame_content, frm));
	}

	virtual void TearDown()
	{
	}

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
