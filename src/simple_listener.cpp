/// \file  simple_listener.cpp
/// \brief Test the BEEP server
///
/// UNCLASSIFIED

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <list>
#include <numeric>
#include <deque>
using namespace std;

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/array.hpp>
#include <boost/asio.hpp>
using boost::bind;
using boost::shared_ptr;
using boost::array;
using namespace boost::asio;
using namespace boost;

#include "beep.hpp"
#include "connection.hpp" // tcp connection
#include "tcp.hpp"
#include "listener.hpp"
#include "test_profile.hpp"

typedef beep::tcptl                                     transport_layer;
typedef beep::basic_session<transport_layer>            session;
typedef beep::basic_listener<session>                   listener;

static char a_global_buffer[4096];
static test_profile testProfile;


static void
handle_channel_data(const beep::reply_code error,
					session &theSession, const beep::channel &info,
					size_t bytes_transferred)
{
	if (error == beep::success) {
		cout << "got some new data at the listener. " << endl;
	} else {
		cerr << "error on channel #" << info.number() << ": " << error << endl;
	}
}

static void
on_sent_channel_init(const beep::reply_code error,
					 session &theSession, const beep::channel &info,
					 size_t bytes_transferred)
{
	cout << "The channel #" << info.number()
		 << " initialization message was transmitted." << endl;
}

static beep::reply_code
handle_new_channel(session &theSession, beep::channel &info, string &init)
{
	cout << "a new channel (#" << info.number()
		 << ") has been created with profile '" << info.profile() << "'."
		 << endl;
	beep::message msg;
	testProfile.initialize(msg);
	theSession.async_send(info, msg, on_sent_channel_init);
	theSession.async_read(info, buffer(a_global_buffer), 
						  handle_channel_data);
	return beep::success;
}
										   

int
main(int argc, char **argv)
{
	try {
		io_service service;
		transport_layer tl(service);
		listener server(tl, ip::tcp::endpoint(ip::tcp::v4(), 12345));

		server.install(testProfile.get_uri(), handle_new_channel);
		service.run();
	} catch (const std::exception &ex) {
		cerr << "Fatal Error: " << ex.what() << endl;
	}
	return 0;
}
