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

static void
handle_channel_change(const boost::system::error_code &error,
					  const unsigned int channel,
					  const bool should_close,
					  const beep::message &init,
					  session_type &theSession)
{
	if (!error) {
		cout << "a new channel (#" << channel
			 << ") has been created with profile 'http://test/profile/usage'."
			 << endl;
		cout << "The peer sent " << init << " as initialization." << endl;
		if (!should_close) {
			beep::message msg;
			msg.set_content("new-channel-payload");
			theSession.send(channel, msg);
			theSession.async_read(channel, handle_channel_data);
		}
	} else {
		cerr << "There was an error changing channel #" << channel << ": " << error.message() << endl;
	}
}

static void handle_new_connection(const boost::system::error_code &error,
								  const beep::identifier &id,
								  session_type &session)
{
	if (!error) {
		session.set_id(id);
	}
}

int
main(int /*argc*/, char **/*argv*/)
{
	using namespace boost::asio;
	using boost::ref;
	using boost::bind;
	try {
		io_service service;
		solo_tcp_listener transport(service);
		session_type session(transport);

		transport.install_network_handler(bind(handle_new_connection,
											   _1, _2, ref(session)));

		session.install_profile("http://test/profile/usage",
								bind(handle_channel_change, _1, _2, _3, _4, ref(session)));

		transport.set_endpoint(ip::tcp::endpoint(ip::tcp::v4(), 12345));
		transport.start_listening();
		service.run();
	} catch (const exception &ex) {
		cerr << "Fatal Error: " << ex.what() << endl;
	}
	return 0;
}
