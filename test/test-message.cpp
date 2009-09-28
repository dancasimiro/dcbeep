/// \file test-message.cpp
///
/// UNCLASSIFIED
#include <gtest/gtest.h>

#include <vector>
#include <iterator>
#include <sstream>

#include "beep/message.hpp"
#include "beep/channel.hpp"
#include "beep/frame.hpp"
#include "beep/frame-generator.hpp"
#include "beep/frame-stream.hpp"

TEST(Message, ContentSetting)
{
	beep::message msg;
	msg.set_content("Test");
	EXPECT_EQ("Content-Type: application/octet-stream\r\n\r\nTest", msg.payload());
	EXPECT_EQ(46u, msg.payload_size());
}

TEST(Message, ChangeMIME)
{
	beep::mime mime;
	mime.set_content_type("application/beep+xml");
	beep::message msg;
	msg.set_content("Test");
	msg.set_mime(mime);
	EXPECT_EQ("Content-Type: application/beep+xml\r\n\r\nTest", msg.payload());
	EXPECT_EQ(42u, msg.payload_size());
}

TEST(Channel, UpdateProperties)
{
	beep::channel ch;
	beep::message msg;
	msg.set_content("Test");
	ch.update(msg);

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

	std::vector<beep::frame> frames;
	EXPECT_EQ(1, beep::make_frames(msg, ch, std::back_inserter(frames)));
	ASSERT_EQ(1u, frames.size());

	const std::string encoded_out =
		"MSG 0 0 . 0 120\r\n"
		"Content-Type: application/beep+xml\r\n"
		"\r\n"
		"<start number='1'>\r\n"
		"   <profile uri='http://iana.org/beep/SASL/OTP' />\r\n"
		"</start>\r\n"
		"END\r\n"
		;
	std::ostringstream strm;
	strm << frames[0];
	EXPECT_EQ(encoded_out, strm.str());

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
