/// \file test-session.cpp
///
/// UNCLASSIFIED
#include <gtest/gtest.h>

#include <boost/asio.hpp>
#include <boost/date_time.hpp>

#include "beep/transport-service/solo-tcp.hpp"
#include "beep/session.hpp"

TEST(ChannelManager, Greeting)
{
	beep::channel_manager chman;
	beep::message greeting;
	beep::profile tls;
	tls.set_uri("http://iana.org/beep/TLS");
	chman.get_greeting_message(&tls, &tls + 1, greeting);
	std::vector<beep::frame> frames;
	EXPECT_EQ(1, beep::make_frames(greeting, chman.get_tuning_channel(),
								   std::back_inserter(frames)));
	ASSERT_EQ(1u, frames.size());

	const std::string encoded_out =
		"RPY 0 0 . 0 101\r\n"
		"Content-Type: application/beep+xml\r\n"
		"\r\n"
		"<greeting><profile uri=\"http://iana.org/beep/TLS\" /></greeting>"
		"END\r\n"
		;
	std::ostringstream strm;
	strm << frames[0];
	EXPECT_EQ(encoded_out, strm.str());

	EXPECT_EQ(0u, chman.get_tuning_channel().get_number());
	EXPECT_EQ(1u, chman.get_tuning_channel().get_message_number());
	EXPECT_EQ(101u, chman.get_tuning_channel().get_sequence_number());
	EXPECT_EQ(0u, chman.get_tuning_channel().get_answer_number());
}

class SessionInitiator : public testing::Test {
public:
	SessionInitiator()
		: testing::Test()
		, service()
		, acceptor(service)
		, socket(service)
		, transport(service)
		, initiator(transport)
		, start_time()
		, buffer()
		, last_error()
		, is_connected(false)
		, have_frame(false)
	{
	}

	virtual ~SessionInitiator() {}

	virtual void SetUp()
	{
		using namespace boost::asio::ip;
		using boost::bind;

		start_time = boost::posix_time::second_clock::local_time();
		buffer.consume(buffer.size());
		is_connected = false;
		last_error = boost::system::error_code();
		socket.close();

		tcp::endpoint ep(address::from_string("127.0.0.1"), 9999);
		acceptor.open(ep.protocol());
		acceptor.set_option(tcp::socket::reuse_address(true));
		acceptor.bind(ep);
		acceptor.listen(1);
		acceptor.async_accept(socket,
							  bind(&SessionInitiator::handle_transport_service_connect,
								   this,
								   boost::asio::placeholders::error));

		transport.set_endpoint(ep);
		ASSERT_NO_THROW(run_event_loop_until_connect());
		ASSERT_TRUE(socket.is_open());
		// wait for the greeting message
		EXPECT_NO_THROW(run_event_loop_until_frame_received());
	}

	virtual void TearDown()
	{
		boost::system::error_code error;
		socket.close(error);
		acceptor.close(error);
		is_connected = false;
		ASSERT_NO_THROW(service.run());
	}

	typedef beep::transport_service::solo_tcp_initiator transport_type;

	boost::asio::io_service             service;
	boost::asio::ip::tcp::acceptor      acceptor;
	boost::asio::ip::tcp::socket        socket;
	transport_type                      transport;
	beep::basic_session<transport_type> initiator;

	boost::posix_time::ptime            start_time;
	boost::asio::streambuf              buffer;
	boost::system::error_code           last_error;
	bool                                is_connected;
	bool                                have_frame;

	void run_event_loop_until_connect()
	{
		boost::posix_time::ptime current_time = boost::posix_time::second_clock::local_time();
		while (!is_connected && (current_time - start_time) < boost::posix_time::seconds(5)) {
			service.poll();
			current_time = boost::posix_time::second_clock::local_time();
			service.reset();
		}
	}

	void run_event_loop_until_frame_received()
	{
		using boost::bind;
		have_frame = false;
		buffer.consume(buffer.size());
		last_error = boost::system::error_code();
		boost::asio::async_read_until(socket,
									  buffer,
									  "END\r\n",
									  bind(&SessionInitiator::handle_frame_reception,
										   this,
										   boost::asio::placeholders::error,
										   boost::asio::placeholders::bytes_transferred));
		boost::posix_time::ptime current_time = boost::posix_time::second_clock::local_time();
		service.reset();
		while (!last_error && !have_frame && (current_time - start_time) < boost::posix_time::seconds(5)) {
			service.poll();
			current_time = boost::posix_time::second_clock::local_time();
			service.reset();
		}
	}

	void handle_transport_service_connect(const boost::system::error_code &error)
	{
		last_error = error;
		if (!error) {
			is_connected = true;

			std::string greeting =
				"RPY 0 0 . 0 109\r\n"
				"Content-Type: application/beep+xml\r\n"
				"\r\n"
				"<greeting>\r\n"
				"   <profile uri='casimiro.daniel/beep/test' />" // 25
				"</greeting>\r\n"
				"END\r\n";
			boost::asio::write(socket, boost::asio::buffer(greeting));
		}
	}

	void handle_frame_reception(const boost::system::error_code &error,
								std::size_t /*bytes_transferred*/)
	{
		last_error = error;
		if (!error) {
			have_frame = true;
		}
	}
};

TEST_F(SessionInitiator, SendsGreeting)
{
	ASSERT_TRUE(have_frame);
	EXPECT_FALSE(last_error);

	std::istream stream(&buffer);
	beep::frame recvFrame;
	EXPECT_TRUE(stream >> recvFrame);
	
	beep::frame myFrame;
	myFrame.set_header(beep::frame::rpy());
	myFrame.set_channel(0);
	myFrame.set_message(0);
	myFrame.set_more(false);
	myFrame.set_sequence(0);
	myFrame.set_payload("Content-Type: application/beep+xml\r\n\r\n" // 38
						"<greeting />"
						);
	EXPECT_EQ(myFrame, recvFrame);
}

class SessionChannelInitiator : public SessionInitiator {
public:
	SessionChannelInitiator()
		: SessionInitiator()
		, session_channel(0)
	{
	}
	virtual ~SessionChannelInitiator() {}

	virtual void SetUp()
	{
		using boost::bind;

		SessionInitiator::SetUp();

		session_channel = 0;

		std::vector<std::string> supported_profiles;
		EXPECT_EQ(1u, initiator.available_profiles(std::back_inserter(supported_profiles)));
		ASSERT_EQ(1u, supported_profiles.size());
		ASSERT_EQ("casimiro.daniel/beep/test", supported_profiles.front());

		const unsigned int channel =
			initiator.async_add_channel("casimiro.daniel/beep/test",
										bind(&SessionChannelInitiator::handle_channel_ready, this,
											 _1, _2));
		EXPECT_EQ(1u, channel);

		EXPECT_NO_THROW(run_event_loop_until_frame_received()); // Get the start message
		EXPECT_FALSE(last_error);

		std::istream stream(&buffer);
		beep::frame recvFrame;
		EXPECT_TRUE(stream >> recvFrame);

		std::ostringstream content_strm;
		content_strm << "Content-Type: application/beep+xml\r\n\r\n" // 38
					 << "<start number=\"1\" serverName=\""
					 << initiator.id() << "\">"
					 <<	"<profile uri=\"casimiro.daniel/beep/test\" />"
					 << "</start>";

		beep::frame myFrame;
		myFrame.set_header(beep::frame::msg());
		myFrame.set_channel(0);
		myFrame.set_message(1);
		myFrame.set_more(false);
		myFrame.set_sequence(50);
		myFrame.set_payload(content_strm.str());
		EXPECT_EQ(myFrame, recvFrame);

		const std::string ok_message =
			"RPY 0 1 . 0 44\r\n"
			"Content-Type: application/beep+xml\r\n\r\n" // 38
			"<ok />" // 6
			"END\r\n";
		boost::asio::write(socket, boost::asio::buffer(ok_message));
		ASSERT_NO_THROW(run_event_loop_until_channel_ready());
	}

	virtual void TearDown()
	{
		SessionInitiator::TearDown();
	}

	unsigned int session_channel;

	void handle_channel_ready(const boost::system::error_code &error,
							  unsigned int channel)
	{
		last_error = error;
		if (!error) {
			session_channel = channel;
		}
	}

	void run_event_loop_until_channel_ready()
	{
		boost::posix_time::ptime current_time = boost::posix_time::second_clock::local_time();
		service.reset();
		while (!last_error && !session_channel && (current_time - start_time) < boost::posix_time::seconds(5)) {
			service.poll();
			current_time = boost::posix_time::second_clock::local_time();
			service.reset();
		}
	}
};

TEST_F(SessionChannelInitiator, StartChannel)
{
	EXPECT_EQ(1u, session_channel);
}

TEST_F(SessionChannelInitiator, PeerClosesChannel)
{
	const std::string close_message =
		"MSG 0 2 . 0 71\r\n"
		"Content-Type: application/beep+xml\r\n\r\n" // 38
		"<close number='1' code='200' />\r\n"
		"END\r\n";
	boost::asio::write(socket, boost::asio::buffer(close_message));

	EXPECT_NO_THROW(run_event_loop_until_frame_received()); // Get the ok message

	std::istream stream(&buffer);
	beep::frame recvFrame;
	EXPECT_TRUE(stream >> recvFrame);

	beep::frame okFrame;
	okFrame.set_header(beep::frame::rpy());
	okFrame.set_channel(0);
	okFrame.set_message(2);
	okFrame.set_more(false);
	okFrame.set_sequence(207);
	okFrame.set_payload("Content-Type: application/beep+xml\r\n\r\n<ok />");
	EXPECT_EQ(okFrame, recvFrame);
}

int
main(int argc, char **argv)
{
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
