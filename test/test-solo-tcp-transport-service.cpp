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

using boost::get;

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
		, session_id()
	{
	}

	virtual ~SingleTCPTransportServiceInitiator() {}

	virtual void SetUp()
	{
		using namespace boost::asio::ip;
		using boost::bind;

		session_id = beep::identifier();
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
	beep::identifier                            session_id;

	void network_is_ready(const boost::system::error_code &error, const beep::identifier &id)
	{
		using beep::transport_service::solo_tcp_initiator;
		using boost::bind;

		last_error = error;
		if (!error) {
			session_id = id;
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
			service.poll_one();
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
	beep::msg_frame myFrame;
	myFrame.channel = 9;
	myFrame.message = 1;
	myFrame.more = false;
	myFrame.sequence = 52;
	myFrame.payload =
		"Content-Type: application/beep+xml\r\n\r\n" // 38
		"<start number='1'>\r\n" // 20
		"   <profile uri='http://iana.org/beep/SASL/OTP' />\r\n" // 52
		"</start>\r\n" // 10
		;
	ts.send_frame(session_id, myFrame);
	ASSERT_NO_THROW(run_event_loop_until_frame_received());
	ASSERT_TRUE(have_frame);
	EXPECT_FALSE(last_error);

	std::istream stream(&buffer);
	beep::frame recvFrame;
	stream >> recvFrame;
	EXPECT_NO_THROW(EXPECT_EQ(myFrame, get<beep::msg_frame>(recvFrame)));
}

TEST_F(SingleTCPTransportServiceInitiator, SendsMultipleFrames)
{
	using boost::bind;

	std::vector<beep::frame> multiple_frames;
	beep::msg_frame firstFrame;
	firstFrame.channel = 9;
	firstFrame.message = 1;
	firstFrame.more = true;
	firstFrame.sequence = 52;
	firstFrame.payload =
		"Content-Type: application/beep+xml\r\n\r\n" // 38
		"<start number='1'>\r\n" // 20
		"   <profile uri='http://iana.org/beep/SASL/OTP' />\r\n" // 52
		;
	multiple_frames.push_back(firstFrame);

	beep::msg_frame secondFrame;
	secondFrame.channel = 9;
	secondFrame.message = 1;
	secondFrame.more = false;
	secondFrame.sequence = 52 + 110;
	secondFrame.payload = "</start>\r\n"; // 10
	multiple_frames.push_back(secondFrame);

	ts.send_frames(session_id, multiple_frames.begin(), multiple_frames.end());
	ASSERT_NO_THROW(run_event_loop_until_frame_received());
	ASSERT_TRUE(have_frame);
	EXPECT_EQ(boost::system::error_code(), last_error);

	// make sure that both frames are received...

	ASSERT_NO_THROW(run_event_loop_until_frame_received());
	ASSERT_TRUE(have_frame);
	EXPECT_EQ(boost::system::error_code(), last_error);

	std::istream stream(&buffer);
#if 0
	while (stream) {
		std::string my_peek;
		std::getline(stream, my_peek);
		std::cerr << my_peek << "\n";
	}
#endif
	beep::frame recvFrame1, recvFrame2;
	stream >> recvFrame1 >> recvFrame2;
	EXPECT_NO_THROW(EXPECT_EQ(firstFrame, get<beep::msg_frame>(recvFrame1)));
	EXPECT_NO_THROW(EXPECT_EQ(secondFrame, get<beep::msg_frame>(recvFrame2)));
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

	beep::msg_frame expectedFrame;
	expectedFrame.channel = 9;
	expectedFrame.message = 1;
	expectedFrame.more = false;
	expectedFrame.sequence = 52;
	expectedFrame.payload =
		"Content-Type: application/beep+xml\r\n\r\n" // 38
		"<start number='1'>\r\n" // 20
		"   <profile uri='http://iana.org/beep/SASL/OTP' />\r\n" // 52
		"</start>\r\n" // 10
		;
	ASSERT_NO_THROW(run_event_loop_until_new_frame());
	EXPECT_TRUE(got_new_frame);
	EXPECT_EQ(boost::system::error_code(), last_error);
	EXPECT_NO_THROW(EXPECT_EQ(expectedFrame, get<beep::msg_frame>(last_frame)));
}

TEST_F(SingleTCPTransportServiceInitiator, ReceivesMultipleFrames)
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
		"END\r\n"

		"MSG 9 2 . 99 120\r\n"
		"Content-Type: application/beep+xml\r\n\r\n" // 38
		"<start number='2'>\r\n" // 20
		"   <profile uri='http://iana.org/beep/SASL/OTP' />\r\n" // 52
		"</start>\r\n" // 10
		"END\r\n"
		;

	async_write(socket, boost::asio::buffer(test_payload),
				bind(&SingleTCPTransportServiceInitiator::handle_test_server_send,
					 this,
					 boost::asio::placeholders::error,
					 boost::asio::placeholders::bytes_transferred));

	beep::msg_frame expectedFrame;
	expectedFrame.channel = 9;
	expectedFrame.message = 1;
	expectedFrame.more = false;
	expectedFrame.sequence = 52;
	expectedFrame.payload =
		"Content-Type: application/beep+xml\r\n\r\n" // 38
		"<start number='1'>\r\n" // 20
		"   <profile uri='http://iana.org/beep/SASL/OTP' />\r\n" // 52
		"</start>\r\n" // 10
		;
	ASSERT_NO_THROW(run_event_loop_until_new_frame());
	EXPECT_TRUE(got_new_frame);
	EXPECT_EQ(boost::system::error_code(), last_error);
	EXPECT_NO_THROW(EXPECT_EQ(expectedFrame, get<beep::msg_frame>(last_frame)));

	got_new_frame = false;
	last_frame = beep::frame();

	beep::msg_frame expectedFrame2;
	expectedFrame2.channel = 9;
	expectedFrame2.message = 2;
	expectedFrame2.more = false;
	expectedFrame2.sequence = 99;
	expectedFrame2.payload =
		"Content-Type: application/beep+xml\r\n\r\n" // 38
		"<start number='2'>\r\n" // 20
		"   <profile uri='http://iana.org/beep/SASL/OTP' />\r\n" // 52
		"</start>\r\n" // 10
		;

	ASSERT_NO_THROW(run_event_loop_until_new_frame());
	EXPECT_TRUE(got_new_frame);
	EXPECT_EQ(boost::system::error_code(), last_error);
	EXPECT_NO_THROW(EXPECT_EQ(expectedFrame2, get<beep::msg_frame>(last_frame)));
}

int
main(int argc, char **argv)
{
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
