/// \file  simple_initiator.cpp
/// \brief Test the BEEP client
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
#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
using boost::bind;
using boost::shared_ptr;
using namespace boost::asio;
using namespace boost;

#include "beep/beep.hpp"
#include "beep/connection.hpp"
#include "beep/tcp.hpp"
#include "beep/initiator.hpp"
#include "test_profile.hpp"

typedef beep::tcptl                                     transport_layer;
typedef beep::basic_session<transport_layer>            session;
typedef beep::basic_initiator<session>                  initiator;

static char a_global_buffer[4096];
static test_profile testProfile;


static beep::reply_code
handle_new_channel(session &theSession, beep::channel &info, string &init)
{
	cout << "a new channel (#" << info.number()
		 << ") has been created with profile '" << info.profile() << "'."
		 << endl;
	return beep::success;
}

static void
on_session_stopped(const beep::reply_code status,
				   session &theSession, const beep::channel &info)
{
	cout << "The session was stopped." << endl;
}

static void
on_channel_closed(const beep::reply_code status,
				  session &theSession, const beep::channel &info)
{
	cout << "The test channel was closed." << endl;
	theSession.stop(on_session_stopped);
}

static void
on_got_data(const beep::reply_code status, 
			session &theSession, const beep::channel &theChannel,
			size_t bytes_transferred)
{
	if (status == beep::success) {
		cout << "The initiator got " << bytes_transferred
			 << " bytes of application data on channel " << theChannel.number()
			 << "!" << endl;
		a_global_buffer[bytes_transferred] = '\0';
		cout << "Contents:\n";// << a_global_buffer << endl;
		std::string line;
		std::string contents(a_global_buffer, bytes_transferred);
		std::istringstream strm(contents);
		while (std::getline(strm, line)) {
			cout << line << endl;
		}
	} else {
		cerr << "Error receiving application data: " << status << "\n";
	}
	if (!theSession.remove_channel(theChannel, on_channel_closed)) {
		cerr << "Failed to remove the channel." << endl;
	}
}

static void
on_channel_created(const beep::reply_code error,
				   session &mySession, const beep::channel &info)
{
	if (error == beep::success) {
		cout << "The test channel is ready; read some data..." << endl;
		mySession.async_read(info, buffer(a_global_buffer), on_got_data);
	} else { 
		cerr << "Failed to create the channel: " << error << endl;
	}
}


static void
on_connect(const boost::system::error_code &error, session &theSession)
{
	if (!error) {
		const int chNum =
			theSession.add_channel(testProfile.get_uri(), on_channel_created);
		cout << "Requested a new channel (#" << chNum << ") with profile '"
			 << testProfile.get_uri() << "'." << endl;
	} else {
		cerr << "The connection failed: " << error.message() << endl;
	}
}

int
main(int argc, char **argv)
{
	try {
		io_service service;
		transport_layer tl(service);
		initiator client(tl);

		client.next_layer().install(testProfile.get_uri(), handle_new_channel);
		client.async_connect(ip::tcp::endpoint(ip::address::from_string("127.0.0.1"),
											   12345), on_connect);
		service.run();
	} catch (const std::exception &ex) {
		cerr << "Fatal Error: " << ex.what() << endl;
	}
	return 0;
}
