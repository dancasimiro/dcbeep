/// \file test-message.cpp
///
/// UNCLASSIFIED
#include <gtest/gtest.h>

#include <vector>
#include <sstream>

#include "beep/message.hpp"
#include "beep/channel.hpp"
#include "beep/frame.hpp"
#include "beep/frame-generator.hpp"
#include "beep/frame-stream.hpp"
#include "beep/message-stream.hpp"

TEST(Message, ContentSetting)
{
	beep::message msg;
	msg.set_content("Test");
	EXPECT_EQ("Content-Type: application/octet-stream\r\n\r\nTest", msg.get_payload());
	EXPECT_EQ(46u, msg.get_payload_size());
}

TEST(Message, ChangeMIME)
{
	beep::mime mime;
	mime.set_content_type("application/beep+xml");
	beep::message msg;
	msg.set_content("Test");
	msg.set_mime(mime);
	EXPECT_EQ("Content-Type: application/beep+xml\r\n\r\nTest", msg.get_payload());
	EXPECT_EQ(42u, msg.get_payload_size());
}

TEST(Message, TextStreamInsertion)
{
	std::istringstream stream("Content-Type: application/beep+xml\r\n\r\nTest-Content");
	beep::message msg;
	EXPECT_TRUE(stream >> msg);
	EXPECT_EQ("Content-Type: application/beep+xml", msg.get_mime().get_content_type());
	EXPECT_EQ("Test-Content", msg.get_content());
}

TEST(Message, TextStreamInsertionWithMissingMIME)
{
	std::istringstream stream("Test-Content");
	beep::message msg;
	EXPECT_FALSE(stream >> msg);
	EXPECT_TRUE(stream.eof());
	EXPECT_EQ("Content-Type: application/octet-stream", msg.get_mime().get_content_type());
	EXPECT_EQ("Test-Content", msg.get_content());
}

TEST(Message, BinaryStreamInsertion)
{
	const int aNumber = 9;
	std::ostringstream ostream;
	ostream << "content-type: application/octet-stream\r\n\r\n";
	EXPECT_TRUE(ostream.write(reinterpret_cast<const char*>(&aNumber), sizeof(int)));

	std::istringstream stream(ostream.str());
	beep::message msg;
	EXPECT_TRUE(stream >> msg);

	EXPECT_EQ("Content-Type: application/octet-stream", msg.get_mime().get_content_type());
	std::istringstream is(msg.get_content());
	int myNumber;
	EXPECT_TRUE(is.read(reinterpret_cast<char*>(&myNumber), sizeof(int)));
	EXPECT_EQ(9, myNumber);
}

TEST(Channel, UpdateProperties)
{
	beep::channel ch;
	beep::message msg;
	msg.set_content("Test");
	ch.update(msg.get_payload_size());

	EXPECT_EQ(0u, ch.get_number());
	EXPECT_EQ(1u, ch.get_message_number());
	EXPECT_EQ(46u, ch.get_sequence_number());
	EXPECT_EQ(0u, ch.get_answer_number());
}

TEST(FrameGenerator, GetFrames)
{
	beep::channel ch;

	beep::message msg;
	msg.set_mime(beep::mime::beep_xml());
	msg.set_content("<start number='1'>\r\n"
					"   <profile uri='http://iana.org/beep/SASL/OTP' />\r\n"
					"</start>\r\n"
					);
	msg.set_channel(ch);

	std::vector<beep::frame> frames;
	EXPECT_NO_THROW(beep::make_frames(msg, std::back_inserter(frames)));
	ASSERT_EQ(1u, frames.size());

	ch.update(msg.get_payload_size());

	beep::msg_frame expected_frame;
	expected_frame.channel = 0;
	expected_frame.message = 0;
	expected_frame.more = false;
	expected_frame.sequence = 0;
	expected_frame.payload = "Content-Type: application/beep+xml\r\n"
		"\r\n"
		"<start number='1'>\r\n"
		"   <profile uri='http://iana.org/beep/SASL/OTP' />\r\n"
		"</start>\r\n"
		;
	EXPECT_NO_THROW(EXPECT_EQ(expected_frame, boost::get<beep::msg_frame>(frames[0])));

	EXPECT_EQ(0u, ch.get_number());
	EXPECT_EQ(1u, ch.get_message_number());
	EXPECT_EQ(120u, ch.get_sequence_number());
	EXPECT_EQ(0u, ch.get_answer_number());
}

int
main(int argc, char **argv)
{
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
