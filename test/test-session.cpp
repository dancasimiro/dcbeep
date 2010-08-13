/// \file test-session.cpp
///
/// UNCLASSIFIED
#define BOOST_SPIRIT_DEBUG 1
#include <gtest/gtest.h>

#include <boost/asio.hpp>
#include <boost/date_time.hpp>

#include "beep/transport-service/solo-tcp.hpp"
#include "beep/session.hpp"
#include "beep/message-stream.hpp"

using boost::get;

static void
handle_initiator_channel_change(const boost::system::error_code &/*error*/,
								const unsigned int /*channel*/,
								const bool /*should_close*/,
								const beep::message &/*msg*/)
{
}

TEST(ChannelManager, Greeting)
{
	beep::channel_manager chman;
	chman.install_profile("http://iana.org/beep/TLS", handle_initiator_channel_change);
	const beep::cmp::protocol_node request = chman.get_greeting_message();
	beep::message greeting = beep::cmp::generate(request);
	chman.prepare_message_for_channel(0, greeting);
	std::vector<beep::frame> frames;
	EXPECT_NO_THROW(beep::make_frames(greeting, std::back_inserter(frames)));
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
}

TEST(ChannelManager, GreetingWithMultipleProfiles)
{
	beep::channel_manager chman;
	chman.install_profile("http://iana.org/beep/TLS", handle_initiator_channel_change);
	chman.install_profile("http://iana.org/beep/TLA", handle_initiator_channel_change);
	const beep::cmp::protocol_node request = chman.get_greeting_message();
	beep::message greeting = beep::cmp::generate(request);
	chman.prepare_message_for_channel(0, greeting);
	std::vector<beep::frame> frames;
	EXPECT_NO_THROW(beep::make_frames(greeting, std::back_inserter(frames)));
	ASSERT_EQ(1u, frames.size());

	const std::string encoded_out =
		"RPY 0 0 . 0 143\r\n"
		"Content-Type: application/beep+xml\r\n"
		"\r\n"
		"<greeting><profile uri=\"http://iana.org/beep/TLS\" /><profile uri=\"http://iana.org/beep/TLA\" /></greeting>"
		"END\r\n"
		;
	std::ostringstream strm;
	strm << frames[0];
	EXPECT_EQ(encoded_out, strm.str());
}

class TimedSessionBase : public testing::Test {
public:
	TimedSessionBase()
		: testing::Test()
		, service()
		, socket(service)
		, start_time()
		, buffer()
		, last_error()
		, last_frame()
		, grammar_()
		, is_connected(false)
		, have_frame(false)
	{
	}
	virtual ~TimedSessionBase() {}

	virtual void SetUp()
	{
		start_time = boost::posix_time::second_clock::local_time();
		buffer.consume(buffer.size());
		is_connected = have_frame = false;
		last_error = boost::system::error_code();
		last_frame = beep::frame();
		socket.close();
	}

	virtual void TearDown()
	{
		boost::system::error_code error;
		socket.close(error);
		is_connected = false;
		ASSERT_NO_THROW(service.run());
	}

	boost::asio::io_service      service;
	boost::asio::ip::tcp::socket socket;
	boost::posix_time::ptime     start_time;
	boost::asio::streambuf       buffer;
	boost::system::error_code    last_error;
	beep::frame                  last_frame;
	beep::frame_parser<boost::spirit::istream_iterator> grammar_;
	bool                         is_connected;
	bool                         have_frame;

	void check_if_timed_out()
	{
		const boost::posix_time::ptime current_time = boost::posix_time::second_clock::local_time();
		if ((current_time - start_time) > boost::posix_time::seconds(5)) {
			using namespace boost::system::errc;
			last_error = make_error_code(timed_out);
		}
	}

	void run_event_loop_until_connect()
	{
		while (!last_error && !is_connected) {
			service.poll();
			service.reset();
			check_if_timed_out();
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
									  bind(&TimedSessionBase::handle_frame_reception,
										   this,
										   boost::asio::placeholders::error,
										   boost::asio::placeholders::bytes_transferred));
		service.reset();
		while (!last_error && !have_frame) {
			service.poll();
			service.reset();
			check_if_timed_out();
		}
	}
private:
	void handle_frame_reception(const boost::system::error_code &error,
								std::size_t /*bytes_transferred*/)
	{
		last_error = error;
		if (!error) {
			std::istream stream(&buffer);
			stream.unsetf(std::ios::skipws);
			boost::spirit::istream_iterator begin(stream);
			boost::spirit::istream_iterator end;
			if (parse(begin, end, grammar_, last_frame)) {
				stream.unget();
				beep::seq_frame * const pseq = boost::get<beep::seq_frame>(&last_frame);
				if (!pseq) { // is not a SEQ frame
					have_frame = true;
				} else {
					boost::asio::async_read_until(socket,
												  buffer,
												  "END\r\n",
												  bind(&TimedSessionBase::handle_frame_reception,
													   this,
													   boost::asio::placeholders::error,
													   boost::asio::placeholders::bytes_transferred));
				}
			}
		}
	}
};

class SessionInitiator : public TimedSessionBase {
public:
	SessionInitiator()
		: TimedSessionBase()
		, acceptor(service)
		, transport(service)
		, initiator(transport)
		, netchng()
	{
	}

	virtual ~SessionInitiator() {}

	virtual void SetUp()
	{
		using namespace boost::asio::ip;
		using boost::bind;

		TimedSessionBase::SetUp();

		netchng = transport.install_network_handler(bind(&SessionInitiator::handle_transport_connect,
														 this, _1, _2));

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
		netchng.disconnect();
		boost::system::error_code error;
		acceptor.close(error);
		TimedSessionBase::TearDown();
	}

	typedef beep::transport_service::solo_tcp_initiator transport_type;

	boost::asio::ip::tcp::acceptor      acceptor;
	transport_type                      transport;
	beep::basic_session<transport_type> initiator;
	boost::signals2::connection         netchng;

private:
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

	void handle_transport_connect(const boost::system::error_code &error,
								  const beep::identifier &id)
	{
		last_error = error;
		if (!error) {
			initiator.set_id(id);
		}
	}
};

TEST_F(SessionInitiator, SendsGreeting)
{
	ASSERT_TRUE(have_frame);
	EXPECT_FALSE(last_error);

	beep::rpy_frame myFrame;
	myFrame.channel = 0;
	myFrame.message = 0;
	myFrame.more = false;
	myFrame.sequence = 0;
	myFrame.payload =
		"Content-Type: application/beep+xml\r\n\r\n" // 38
		"<greeting />"
		;
	EXPECT_NO_THROW(EXPECT_EQ(myFrame, get<beep::rpy_frame>(last_frame)));
}

class SessionChannelInitiator : public SessionInitiator {
public:
	SessionChannelInitiator()
		: SessionInitiator()
		, session_channel(0)
		, user_read(false)
	{
	}
	virtual ~SessionChannelInitiator() {}

	virtual void SetUp()
	{
		using boost::bind;

		SessionInitiator::SetUp();

		session_channel = 0;
		user_read = false;
		user_message = beep::message();

		// This list is sent from the listening peer
		std::vector<std::string> supported_profiles;
		EXPECT_NO_THROW(initiator.available_profiles(std::back_inserter(supported_profiles)));
		ASSERT_EQ(1u, supported_profiles.size());
		ASSERT_EQ("casimiro.daniel/beep/test", supported_profiles.front());

		ASSERT_NO_THROW(initiator.install_profile("casimiro.daniel/beep/test",
												  handle_initiator_channel_change));

		const unsigned int channel =
			initiator.async_add_channel("casimiro.daniel/beep/test",
										bind(&SessionChannelInitiator::handle_channel_ready, this,
											 _1, _2));
		EXPECT_EQ(1u, channel);

		EXPECT_NO_THROW(run_event_loop_until_frame_received()); // Get the start message
		EXPECT_FALSE(last_error);

		std::ostringstream content_strm;
		content_strm << "Content-Type: application/beep+xml\r\n\r\n" // 38
					 << "<start number=\"1\" serverName=\""
					 << initiator.id() << "\">"
					 <<	"<profile uri=\"casimiro.daniel/beep/test\" />"
					 << "</start>";

		beep::msg_frame myFrame;
		myFrame.channel = 0;
		myFrame.message = 1;
		myFrame.more = false;
		myFrame.sequence = 50;
		myFrame.payload = content_strm.str();
		EXPECT_NO_THROW(EXPECT_EQ(myFrame, get<beep::msg_frame>(last_frame)));

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

	unsigned int  session_channel;
	bool          user_read;
	beep::message user_message;

	void handle_channel_ready(const boost::system::error_code &error,
							  unsigned int channel)
	{
		last_error = error;
		if (!error) {
			session_channel = channel;
		}
	}

	void handle_user_read(const boost::system::error_code &error,
						  const beep::message &msg,
						  const unsigned int channel)
	{
		last_error = error;
		if (!error) {
			user_read = true;
			session_channel = channel;
			user_message = msg;
		}
	}

	void handle_channel_close(const boost::system::error_code &error,
							  const unsigned int channel)
	{
		last_error = error;
		if (!error) {
			session_channel = channel;
		}
	}

	void run_event_loop_until_channel_ready()
	{
		service.reset();
		while (!last_error && !session_channel) {
			service.poll();
			service.reset();
			check_if_timed_out();
		}
	}

	void run_event_loop_until_user_read()
	{
		service.reset();
		user_read = false;
		while (!last_error && !user_read) {
			service.poll();
			service.reset();
			check_if_timed_out();
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

	beep::rpy_frame okFrame;
	okFrame.channel = 0;
	okFrame.message = 2;
	okFrame.more = false;
	okFrame.sequence = 207;
	okFrame.payload = "Content-Type: application/beep+xml\r\n\r\n<ok />";
	EXPECT_NO_THROW(EXPECT_EQ(okFrame, get<beep::rpy_frame>(last_frame)));
}

TEST_F(SessionChannelInitiator, ClosesChannel)
{
	using boost::bind;
	ASSERT_EQ(1u, session_channel);
	initiator.async_close_channel(session_channel,
								  beep::reply_code::success,
								  bind(&SessionChannelInitiator::handle_channel_close,
									   this,
									   _1, _2));
	EXPECT_NO_THROW(run_event_loop_until_frame_received()); // Get the ok message

	EXPECT_FALSE(last_error);
	session_channel = 0;

	beep::msg_frame closeFrame;
	closeFrame.channel = 0;
	closeFrame.message = 2;
	closeFrame.more = false;
	closeFrame.sequence = 207;
	closeFrame.payload = "Content-Type: application/beep+xml\r\n\r\n<close number=\"1\" code=\"200\" />";
	EXPECT_NO_THROW(EXPECT_EQ(closeFrame, get<beep::msg_frame>(last_frame)));

	// The message number in ok_message must match the closeFrame
	const std::string ok_message =
		"RPY 0 2 . 0 44\r\n"
		"Content-Type: application/beep+xml\r\n\r\n" // 38
		"<ok />" // 6
		"END\r\n";
	boost::asio::write(socket, boost::asio::buffer(ok_message));

	ASSERT_NO_THROW(run_event_loop_until_channel_ready());
	EXPECT_EQ(1u, session_channel);
	EXPECT_FALSE(last_error);
}

TEST_F(SessionChannelInitiator, AsyncRead)
{
	using boost::bind;
	initiator.async_read(session_channel,
						 bind(&SessionChannelInitiator::handle_user_read,
							  this,
							  _1, _2, _3));
	const std::string payload =
		"MSG 1 0 . 0 12\r\nTest PayloadEND\r\n";
	boost::asio::write(socket, boost::asio::buffer(payload));
	EXPECT_NO_THROW(run_event_loop_until_user_read());

	EXPECT_EQ(1u, session_channel);
	beep::message expected;
	expected.set_content("Test Payload");
	EXPECT_EQ(expected, user_message);
	EXPECT_EQ(expected.get_content(), user_message.get_content());
}

TEST_F(SessionChannelInitiator, AsyncWrite)
{
	using boost::bind;
	beep::message msg;
	msg.set_content("Test Payload");

	initiator.send(session_channel, msg);
	ASSERT_NO_THROW(run_event_loop_until_frame_received());
	EXPECT_FALSE(last_error);
	EXPECT_EQ(1u, session_channel);

	beep::msg_frame expectedFrame;
	expectedFrame.channel = session_channel;
	expectedFrame.message = 0;
	expectedFrame.more = false;
	expectedFrame.sequence = 0;
	expectedFrame.payload = "Content-Type: application/octet-stream\r\n\r\nTest Payload";
	EXPECT_NO_THROW(EXPECT_EQ(expectedFrame, get<beep::msg_frame>(last_frame)));
}

class SessionListener : public TimedSessionBase {
public:
	SessionListener()
		: TimedSessionBase()
		, transport(service)
		, listener(transport)
		, netchng()
		, session_channel(0)
		, user_message()
	{
	}

	virtual ~SessionListener() {}

	typedef beep::transport_service::solo_tcp_listener transport_type;

	transport_type                      transport;
	beep::basic_session<transport_type> listener;
	boost::signals2::connection         netchng;
	unsigned int                        session_channel;
	beep::message                       user_message;

	virtual void SetUp()
	{
		using namespace boost::asio;
		using boost::bind;

		TimedSessionBase::SetUp();

		session_channel = 0;
		user_message = beep::message();

		netchng = transport.install_network_handler(bind(&SessionListener::handle_transport_connect,
														 this, _1, _2));

		listener.install_profile("casimiro.daniel/test-profile",
								 bind(&SessionListener::handle_test_profile_change,
									  this, _1, _2, _3, _4));

		ip::tcp::endpoint ep(ip::address::from_string("127.0.0.1"), 9999);
		transport.set_endpoint(ep);
		transport.start_listening();
		socket.async_connect(ep, bind(&SessionListener::handle_connect, this,
									  placeholders::error));
		ASSERT_NO_THROW(run_event_loop_until_connect());
		ASSERT_TRUE(socket.is_open());
		// wait for the greeting message
		EXPECT_NO_THROW(run_event_loop_until_frame_received());
	}

	virtual void TearDown()
	{
		netchng.disconnect();
		transport.stop_listening();
		beep::shutdown_session(listener);
		TimedSessionBase::TearDown();
	}

	void handle_connect(const boost::system::error_code &error)
	{
		last_error = error;
		if (!error) {
			is_connected = true;

			/// send the greeting
			const std::string greeting =
				"RPY 0 0 . 0 50\r\n"
				"Content-Type: application/beep+xml\r\n\r\n" // 38
				"<greeting />"
				"END\r\n";
			boost::asio::write(socket, boost::asio::buffer(greeting));
		}
	}

	void handle_transport_connect(const boost::system::error_code &error,
								  const beep::identifier &id)
	{
		last_error = error;
		if (!error) {
			listener.set_id(id);
		}
	}

	void handle_test_profile_change(const boost::system::error_code& error,
									const unsigned int channel,
									const bool should_close,
									const beep::message &init)
	{
		if (!error) {
			if (!should_close) {
				session_channel = channel;
				user_message = init;
			}
		}
	}
};

TEST_F(SessionListener, SendsGreeting)
{
	ASSERT_TRUE(have_frame);
	EXPECT_FALSE(last_error);

	beep::rpy_frame myFrame;
	myFrame.channel = 0;
	myFrame.message = 0;
	myFrame.more = false;
	myFrame.sequence = 0;
	myFrame.payload =
		"Content-Type: application/beep+xml\r\n\r\n" // 38
		"<greeting>"
		"<profile uri=\"casimiro.daniel/test-profile\" />"
		"</greeting>"
		;
	EXPECT_NO_THROW(EXPECT_EQ(myFrame, get<beep::rpy_frame>(last_frame)));
}

class SessionChannelListener : public SessionListener {
public:
	SessionChannelListener()
		: SessionListener()
		, user_read(false)
	{
	}

	virtual ~SessionChannelListener() {}

	bool          user_read;

	virtual void SetUp()
	{
		SessionListener::SetUp();

		user_read = false;

		std::string start_channel_payload =
			"MSG 0 1 . 0 110\r\n"
			"Content-Type: application/beep+xml\r\n\r\n"       // 38
			"<start number=\"1\">"                             // 18
			"<profile uri=\"casimiro.daniel/test-profile\" />" // 46
			"</start>"                                         // 8
			"END\r\n";
		boost::asio::write(socket, boost::asio::buffer(start_channel_payload));
		// wait for the response
		EXPECT_NO_THROW(run_event_loop_until_frame_received());
	}

	virtual void TearDown()
	{
		SessionListener::TearDown();
	}

	void run_event_loop_until_user_read()
	{
		service.reset();
		user_read = false;
		while (!last_error && !user_read) {
			service.poll();
			service.reset();
			check_if_timed_out();
		}
	}

	void run_event_loop_until_channel_ready()
	{
		service.reset();
		while (!last_error && !session_channel) {
			service.poll();
			service.reset();
			check_if_timed_out();
		}
	}

	void handle_user_read(const boost::system::error_code &error,
						  const beep::message &msg,
						  const unsigned int channel)
	{
		last_error = error;
		if (!error) {
			user_read = true;
			session_channel = channel;
			user_message = msg;
		}
	}

	void handle_channel_close(const boost::system::error_code &error,
							  const unsigned int channel)
	{
		last_error = error;
		if (!error) {
			session_channel = channel;
		}
	}
};

TEST_F(SessionChannelListener, StartChannel)
{
	beep::rpy_frame okFrame;
	okFrame.channel = 0;
	okFrame.message = 1;
	okFrame.more = false;
	okFrame.sequence = 105;
	okFrame.payload = "Content-Type: application/beep+xml\r\n\r\n<profile uri=\"casimiro.daniel/test-profile\" />";
	EXPECT_NO_THROW(EXPECT_EQ(okFrame, get<beep::rpy_frame>(last_frame)));
	
	EXPECT_EQ(1u, session_channel);
}

TEST_F(SessionChannelListener, PeerClosesChannel)
{
	const std::string close_message =
		"MSG 0 2 . 0 71\r\n"
		"Content-Type: application/beep+xml\r\n\r\n" // 38
		"<close number='1' code='200' />\r\n"
		"END\r\n";
	boost::asio::write(socket, boost::asio::buffer(close_message));

	EXPECT_NO_THROW(run_event_loop_until_frame_received()); // Get the ok message

	beep::rpy_frame okFrame;
	okFrame.channel = 0;
	okFrame.message = 2;
	okFrame.more = false;
	okFrame.sequence = 189;
	okFrame.payload = "Content-Type: application/beep+xml\r\n\r\n<ok />";
	EXPECT_NO_THROW(EXPECT_EQ(okFrame, get<beep::rpy_frame>(last_frame)));
}

TEST_F(SessionChannelListener, ClosesChannel)
{
	using boost::bind;
	ASSERT_EQ(1u, session_channel);
	listener.async_close_channel(session_channel,
								 beep::reply_code::success,
								 bind(&SessionChannelListener::handle_channel_close,
									  this,
									  _1, _2));
	EXPECT_NO_THROW(run_event_loop_until_frame_received()); // Get the ok message

	EXPECT_FALSE(last_error);
	session_channel = 0;

	beep::msg_frame closeFrame;
	closeFrame.channel = 0;
	closeFrame.message = 2;
	closeFrame.more = false;
	closeFrame.sequence = 189;
	closeFrame.payload = "Content-Type: application/beep+xml\r\n\r\n<close number=\"1\" code=\"200\" />";
	EXPECT_NO_THROW(EXPECT_EQ(closeFrame, get<beep::msg_frame>(last_frame)));

	// The message number in ok_message must match the closeFrame
	const std::string ok_message =
		"RPY 0 2 . 0 44\r\n"
		"Content-Type: application/beep+xml\r\n\r\n" // 38
		"<ok />" // 6
		"END\r\n";
	boost::asio::write(socket, boost::asio::buffer(ok_message));

	ASSERT_NO_THROW(run_event_loop_until_channel_ready());
	EXPECT_EQ(1u, session_channel);
	EXPECT_FALSE(last_error);
}

TEST_F(SessionChannelListener, AsyncRead)
{
	using boost::bind;
	listener.async_read(session_channel,
						bind(&SessionChannelListener::handle_user_read,
							 this,
							 _1, _2, _3));
	const std::string payload =
		"MSG 1 0 . 0 12\r\nTest PayloadEND\r\n";
	boost::asio::write(socket, boost::asio::buffer(payload));
	EXPECT_NO_THROW(run_event_loop_until_user_read());

	EXPECT_EQ(1u, session_channel);
	beep::message expected;
	expected.set_content("Test Payload");
	EXPECT_EQ(expected, user_message);
}

TEST_F(SessionChannelListener, AsyncWrite)
{
	using boost::bind;
	beep::message msg;
	msg.set_content("Test Payload");

	listener.send(session_channel, msg);
	ASSERT_NO_THROW(run_event_loop_until_frame_received());
	EXPECT_FALSE(last_error);
	EXPECT_EQ(1u, session_channel);

	beep::msg_frame expectedFrame;
	expectedFrame.channel = session_channel;
	expectedFrame.message = 0;
	expectedFrame.more = false;
	expectedFrame.sequence = 0;
	expectedFrame.payload = "Content-Type: application/octet-stream\r\n\r\nTest Payload";
	EXPECT_NO_THROW(EXPECT_EQ(expectedFrame, get<beep::msg_frame>(last_frame)));
}

int
main(int argc, char **argv)
{
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
