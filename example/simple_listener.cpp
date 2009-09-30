/// \file  simple_listener.cpp
/// \brief Test the BEEP server
///
/// UNCLASSIFIED
#include "beep/transport-service/solo-tcp.hpp"
#include "beep/session.hpp"

#include <iostream>
#include <stdexcept>
using namespace std;

#include <boost/asio.hpp>
#include <boost/bind.hpp>

using beep::transport_service::solo_tcp_listener;
typedef beep::basic_session<solo_tcp_listener> session_type;

static void
handle_channel_data(const boost::system::error_code &error,
					const beep::message &msg,
					const unsigned int channel)
{
	if (!error) {
		cout << "got some new data (" << msg << ") at the listener. " << endl;
	} else {
		cerr << "error on channel #" << channel << ": " << error << endl;
	}
}
#if 0
static void
on_sent_channel_init(const beep::reply_code error,
					 session &theSession, const beep::channel &info,
					 size_t bytes_transferred)
{
	cout << "The channel #" << info.number()
		 << " initialization message was transmitted." << endl;
}
#endif
static void
handle_new_channel(const unsigned int channel,
				   const beep::message &/*init*/,
				   session_type &theSession)
{
	cout << "a new channel (#" << channel
		 << ") has been created with profile 'http://test/profile/usage'."
		 << endl;
	//testProfile.initialize(msg);
	//theSession.async_send(info, msg, on_sent_channel_init);
	theSession.async_read(channel, handle_channel_data);
}

int
main(int /*argc*/, char **/*argv*/)
{
	using namespace boost::asio;
	using boost::ref;
	using boost::bind;
	try {
		io_service service;
		solo_tcp_listener transport(service, ip::tcp::endpoint(ip::tcp::v4(), 12345));
		session_type session(transport);

		beep::profile myProfile;
		myProfile.set_uri("http://test/profile/usage");
		beep::message init_msg;
		init_msg.set_content("Application Specific Message!\r\n");
		myProfile.set_initialization_message(init_msg);

		session.install_profile(myProfile,
								bind(handle_new_channel, _1, _2, ref(session)));
		service.run();
	} catch (const exception &ex) {
		cerr << "Fatal Error: " << ex.what() << endl;
	}
	return 0;
}
