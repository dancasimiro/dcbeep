/// \file test-cmp.cpp
///
/// UNCLASSIFIED
#include <gtest/gtest.h>

#include <vector>
#include <sstream>

#include "beep/channel-management-protocol.hpp"

using namespace beep::cmp;
using boost::get;

TEST(ChannelManagerParsing, EmptyGreetingMessage)
{
	const std::string input = "<greeting />";
	greeting_message output;
	ASSERT_NO_THROW(output = get<greeting_message>(parse(input)));
	EXPECT_TRUE(output.profile_uris.empty());
	EXPECT_TRUE(output.features.empty());
	EXPECT_TRUE(output.localizations.empty());
}

TEST(ChannelManagerParsing, SingleProfileGreetingMessage)
{
	const std::string input =
		"<greeting><profile uri='http://iana.org/beep/TLS' /></greeting>";
	greeting_message output;
	ASSERT_NO_THROW(output = get<greeting_message>(parse(input)));
	EXPECT_TRUE(output.features.empty());
	EXPECT_TRUE(output.localizations.empty());
	ASSERT_FALSE(output.profile_uris.empty());
	EXPECT_EQ(1u, output.profile_uris.size());
	EXPECT_EQ("http://iana.org/beep/TLS", output.profile_uris[0]);
}

TEST(ChannelManagerParsing, SingleProfileGreetingMessageWithSpace)
{
	const std::string input =
		"<greeting>   <profile \turi=   'http://iana.org/beep/TLS' />\t</greeting>";
	greeting_message output;
	ASSERT_NO_THROW(output = get<greeting_message>(parse(input)));
	EXPECT_TRUE(output.features.empty());
	EXPECT_TRUE(output.localizations.empty());
	ASSERT_FALSE(output.profile_uris.empty());
	EXPECT_EQ(1u, output.profile_uris.size());
	EXPECT_EQ("http://iana.org/beep/TLS", output.profile_uris[0]);
}

TEST(ChannelManagerParsing, SingleProfileGreetingMessageWithNewLine)
{
	const std::string input =
		"<greeting>\n\t<profile\n\n uri=\"http://iana.org/beep/TLS\" />\n</greeting>";
	greeting_message output;
	ASSERT_NO_THROW(output = get<greeting_message>(parse(input)));
	EXPECT_TRUE(output.features.empty());
	EXPECT_TRUE(output.localizations.empty());
	ASSERT_FALSE(output.profile_uris.empty());
	EXPECT_EQ(1u, output.profile_uris.size());
	EXPECT_EQ("http://iana.org/beep/TLS", output.profile_uris[0]);
}

TEST(ChannelManagerParsing, MultipleProfilesGreetingMessageWithNewLine)
{
	const std::string input =
		"<greeting>\n\t<profile\n\n uri=\"http://iana.org/beep/TLS\" />\n"
		"<profile\n\n uri=\"http://iana.org/beep/SASL\" />\n</greeting>";
	greeting_message output;
	ASSERT_NO_THROW(output = get<greeting_message>(parse(input)));
	EXPECT_TRUE(output.features.empty());
	EXPECT_TRUE(output.localizations.empty());
	ASSERT_FALSE(output.profile_uris.empty());
	EXPECT_EQ(2u, output.profile_uris.size());
	EXPECT_EQ("http://iana.org/beep/TLS", output.profile_uris[0]);
	EXPECT_EQ("http://iana.org/beep/SASL", output.profile_uris[1]);
}

TEST(ChannelManagerParsing, StartMessage)
{
	const std::string input =
		"<start number='1'><profile uri='http://iana.org/beep/SASL/OTP' /></start>";
	start_message output;
	ASSERT_NO_THROW(output = get<start_message>(parse(input)));
	EXPECT_EQ(1u, output.channel);
	EXPECT_EQ("", output.server_name);
	ASSERT_EQ(1u, output.profiles.size());
	EXPECT_EQ("http://iana.org/beep/SASL/OTP", output.profiles[0].uri);
}

TEST(ChannelManagerParsing, CloseMessage)
{
	const std::string input = "<close number='1' code='200' />";
	close_message output;
	ASSERT_NO_THROW(output = get<close_message>(parse(input)));
	EXPECT_EQ(1u, output.channel);
	EXPECT_EQ(200u, output.code);
}

TEST(ChannelManagerParsing, OKMessage)
{
	const std::string input = "<ok />";
	ok_message output;
	ASSERT_NO_THROW(output = get<ok_message>(parse(input)));
}

TEST(ChannelManagerParsing, ErrorMessage)
{
	const std::string input = "<error code='550'>all requested profiles are unsupported</error>";
	error_message output;
	ASSERT_NO_THROW(output = get<error_message>(parse(input)));
	EXPECT_EQ(550u, output.code);
	EXPECT_EQ("all requested profiles are unsupported", output.diagnostic);
}

TEST(ChannelManagerParsing, RealErrorMessage)
{
	const std::string input =
		"<error code=\"451\">channel_manager::prepare_message_for_channel -- The selected channel (15) is not in use.</error>";
	error_message output;
	ASSERT_NO_THROW(output = get<error_message>(parse(input)));
	EXPECT_EQ(451u, output.code);
	EXPECT_EQ("channel_manager::prepare_message_for_channel -- The selected channel (15) is not in use.", output.diagnostic);
}

int
main(int argc, char **argv)
{
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
