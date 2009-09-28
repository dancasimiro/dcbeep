/// \file test-session.cpp
///
/// UNCLASSIFIED
#include <gtest/gtest.h>

#include <boost/asio.hpp>
#include <boost/date_time.hpp>

#include "beep/transport-service/solo-tcp.hpp"
#include "beep/session.hpp"

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
		boost::asio::async_read_until(socket,
									  buffer,
									  "END\r\n",
									  bind(&SessionInitiator::handle_frame_reception,
										   this,
										   boost::asio::placeholders::error,
										   boost::asio::placeholders::bytes_transferred));
		boost::posix_time::ptime current_time = boost::posix_time::second_clock::local_time();
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
	EXPECT_NO_THROW(run_event_loop_until_frame_received());
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

int
main(int argc, char **argv)
{
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
