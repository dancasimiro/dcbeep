/// \file test-solo-tcp-transport-service.cpp
///
/// UNCLASSIFIED
//#define BOOST_SPIRIT_DEBUG
#include <gtest/gtest.h>

#include <string>
#include <istream>

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/date_time.hpp>

#include "beep/frame.hpp"
#include "beep/transport-service/solo-tcp.hpp"

///////////////////////////////////////////////////////////////////////////////
// Test the single TCP/IP connection transport service
///////////////////////////////////////////////////////////////////////////////
class SingleTCPTransportServiceInitiator : public testing::Test {
public:
	SingleTCPTransportServiceInitiator()
		: testing::Test()
		, service()
		, acceptor(service)
		, socket(service)
		, ts(service)
		, is_connected(false)
		, have_frame(false)
		, last_error()
		, buffer()
		, start_time()
		, session_connection()
	{
	}

	virtual ~SingleTCPTransportServiceInitiator() {}

	virtual void SetUp()
	{
		using namespace boost::asio::ip;
		using boost::bind;

		start_time = boost::posix_time::second_clock::local_time();
		is_connected = have_frame = got_new_frame = false;
		buffer.consume(buffer.size());
		last_error = boost::system::error_code();
		last_frame = beep::frame();
		socket.close();

		session_connection =
			ts.install_network_handler(bind(&SingleTCPTransportServiceInitiator::network_is_ready,
											this, _1, _2));

		tcp::endpoint ep(address::from_string("127.0.0.1"), 9999);
		acceptor.open(ep.protocol());
		acceptor.set_option(tcp::socket::reuse_address(true));
		acceptor.bind(ep);
		acceptor.listen(1);
		acceptor.async_accept(socket,
							  bind(&SingleTCPTransportServiceInitiator::handle_transport_service_connect,
								   this,
								   boost::asio::placeholders::error));
		ts.set_endpoint(ep);
		ASSERT_NO_THROW(run_event_loop_until_connect());
		ASSERT_TRUE(socket.is_open());
	}

	virtual void TearDown()
	{
		session_connection.disconnect();
		boost::system::error_code error;
		socket.close(error);
		acceptor.close(error);
		is_connected = false;
		ASSERT_NO_THROW(service.run());
	}

	boost::asio::io_service                     service;
	boost::asio::ip::tcp::acceptor              acceptor;
	boost::asio::ip::tcp::socket                socket;
	beep::transport_service::solo_tcp_initiator ts;
	bool                                        is_connected, have_frame, got_new_frame;
	boost::system::error_code                   last_error;
	boost::asio::streambuf                      buffer;
	boost::posix_time::ptime                    start_time;
	beep::frame                                 last_frame;
	boost::signals2::connection                 session_connection;

	void network_is_ready(const boost::system::error_code &error, const beep::identifier &id)
	{
		using beep::transport_service::solo_tcp_initiator;
		using boost::bind;

		last_error = error;
		if (!error) {
			solo_tcp_initiator::signal_connection sigconn =
				ts.subscribe(id,
							 bind(&SingleTCPTransportServiceInitiator::handle_new_frame,
								  this, _1, _2));
			EXPECT_TRUE(sigconn.connected());
		}
	}

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
									  bind(&SingleTCPTransportServiceInitiator::handle_frame_reception,
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

	void run_event_loop_until_new_frame()
	{
		got_new_frame = false;
		boost::posix_time::ptime current_time = boost::posix_time::second_clock::local_time();
		while (!last_error && !got_new_frame && (current_time - start_time) < boost::posix_time::seconds(5)) {
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

	void handle_test_server_send(const boost::system::error_code &error,
								 std::size_t /*bytes_transferred*/)
	{
		last_error = error;
	}

	void handle_frame_reception(const boost::system::error_code &error,
								std::size_t /*bytes_transferred*/)
	{
		last_error = error;
		if (!error) {
			have_frame = true;
		}
	}

	void handle_new_frame(const boost::system::error_code &error,
						  const beep::frame &frame)
	{
		last_error = error;
		if (!error) {
			last_frame = frame;
			got_new_frame = true;
		}
	}
};

TEST_F(SingleTCPTransportServiceInitiator, Connect)
{
	EXPECT_FALSE(last_error);
	EXPECT_TRUE(is_connected);
}

TEST_F(SingleTCPTransportServiceInitiator, SendsProperFrame)
{
	using boost::bind;

	beep::frame myFrame;
	myFrame.set_header(beep::frame::msg());
	myFrame.set_channel(9);
	myFrame.set_message(1);
	myFrame.set_more(false);
	myFrame.set_sequence(52);
	myFrame.set_payload("Content-Type: application/beep+xml\r\n\r\n" // 38
						"<start number='1'>\r\n" // 20
						"   <profile uri='http://iana.org/beep/SASL/OTP' />\r\n" // 52
						"</start>\r\n" // 10
						);
	ts.send_frame(myFrame);
	ASSERT_NO_THROW(run_event_loop_until_frame_received());
	ASSERT_TRUE(have_frame);
	EXPECT_FALSE(last_error);

	std::istream stream(&buffer);
	beep::frame recvFrame;
	EXPECT_TRUE(stream >> recvFrame);
	EXPECT_EQ(myFrame, recvFrame);
}

TEST_F(SingleTCPTransportServiceInitiator, SendsMultipleFrames)
{
	using boost::bind;

	std::vector<beep::frame> multiple_frames;
	beep::frame firstFrame;
	firstFrame.set_header(beep::frame::msg());
	firstFrame.set_channel(9);
	firstFrame.set_message(1);
	firstFrame.set_more(true);
	firstFrame.set_sequence(52);
	firstFrame.set_payload("Content-Type: application/beep+xml\r\n\r\n" // 38
						"<start number='1'>\r\n" // 20
						"   <profile uri='http://iana.org/beep/SASL/OTP' />\r\n" // 52
						);
	multiple_frames.push_back(firstFrame);

	beep::frame secondFrame;
	secondFrame.set_header(beep::frame::msg());
	secondFrame.set_channel(9);
	secondFrame.set_message(2);
	secondFrame.set_more(false);
	secondFrame.set_sequence(52 + 110);
	secondFrame.set_payload("</start>\r\n"); // 10
	multiple_frames.push_back(secondFrame);

	ts.send_frames(multiple_frames.begin(), multiple_frames.end());
	ASSERT_NO_THROW(run_event_loop_until_frame_received());
	ASSERT_TRUE(have_frame);
	EXPECT_FALSE(last_error);

	{
		std::istream stream1(&buffer);
		beep::frame recvFrame1;
		EXPECT_TRUE(stream1 >> recvFrame1);
		EXPECT_EQ(firstFrame, recvFrame1);
	}

	ASSERT_NO_THROW(run_event_loop_until_frame_received());
	ASSERT_TRUE(have_frame);
	EXPECT_FALSE(last_error);

	{
		std::istream stream2(&buffer);
		beep::frame recvFrame2;
		EXPECT_TRUE(stream2 >> recvFrame2);
		EXPECT_EQ(secondFrame, recvFrame2);
	}
}

TEST_F(SingleTCPTransportServiceInitiator, ReceivesProperFrame)
{
	using boost::bind;
	using beep::transport_service::solo_tcp_initiator;
	using boost::asio::async_write;

	const std::string test_payload =
		"MSG 9 1 . 52 120\r\n"
		"Content-Type: application/beep+xml\r\n\r\n" // 38
		"<start number='1'>\r\n" // 20
		"   <profile uri='http://iana.org/beep/SASL/OTP' />\r\n" // 52
		"</start>\r\n" // 10
		"END\r\n";

	async_write(socket, boost::asio::buffer(test_payload),
				bind(&SingleTCPTransportServiceInitiator::handle_test_server_send,
					 this,
					 boost::asio::placeholders::error,
					 boost::asio::placeholders::bytes_transferred));

	beep::frame expectedFrame;
	expectedFrame.set_header(beep::frame::msg());
	expectedFrame.set_channel(9);
	expectedFrame.set_message(1);
	expectedFrame.set_more(false);
	expectedFrame.set_sequence(52);
	expectedFrame.set_payload("Content-Type: application/beep+xml\r\n\r\n" // 38
							  "<start number='1'>\r\n" // 20
							  "   <profile uri='http://iana.org/beep/SASL/OTP' />\r\n" // 52
							  "</start>\r\n" // 10
							  );
	ASSERT_NO_THROW(run_event_loop_until_new_frame());
	EXPECT_TRUE(got_new_frame);
	EXPECT_EQ(boost::system::error_code(), last_error);
	EXPECT_EQ(expectedFrame, last_frame);
}

int
main(int argc, char **argv)
{
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
