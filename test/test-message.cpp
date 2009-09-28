/// \file test-message.cpp
///
/// UNCLASSIFIED
#include <gtest/gtest.h>

#include "beep/message.hpp"
#include "beep/channel.hpp"

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

int
main(int argc, char **argv)
{
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
