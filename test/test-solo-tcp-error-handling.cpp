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
class SingleTCPTransportServiceErrorHandling : public testing::Test {
public:
	SingleTCPTransportServiceErrorHandling()
		: testing::Test()
		, service()
		, acceptor(service)
		, socket(service)
		, ts(service, 16) /*setting max buffer size small to force an error*/
		, is_connected(false)
		, have_frame(false)
		, last_error()
		, buffer()
		, start_time()
		, session_connection()
		, session_id()
	{
	}

	virtual ~SingleTCPTransportServiceErrorHandling() {}

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
			ts.install_network_handler(bind(&SingleTCPTransportServiceErrorHandling::network_is_ready,
											this, _1, _2));

		tcp::endpoint ep(address::from_string("127.0.0.1"), 9999);
		acceptor.open(ep.protocol());
		acceptor.set_option(tcp::socket::reuse_address(true));
		acceptor.bind(ep);
		acceptor.listen(1);
		acceptor.async_accept(socket, bind(&SingleTCPTransportServiceErrorHandling::handle_transport_service_connect,
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
							 bind(&SingleTCPTransportServiceErrorHandling::handle_new_frame,
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
									  bind(&SingleTCPTransportServiceErrorHandling::handle_frame_reception,
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

TEST_F(SingleTCPTransportServiceErrorHandling, Connect)
{
	EXPECT_FALSE(last_error);
	EXPECT_TRUE(is_connected);
}

TEST_F(SingleTCPTransportServiceErrorHandling, HandlesError)
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
	EXPECT_TRUE(last_error);
}

int
main(int argc, char **argv)
{
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
