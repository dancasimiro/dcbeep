/// \file  simple_initiator.cpp
/// \brief Test the BEEP client
///
/// UNCLASSIFIED
#include "beep/transport-service/solo-tcp.hpp"
#include "beep/session.hpp"

#include <iostream>
#include <string>
#include <vector>
using namespace std;

#include <boost/bind.hpp>

using beep::transport_service::solo_tcp_initiator;
typedef beep::basic_session<solo_tcp_initiator> session_type;

static void
on_channel_closed(const boost::system::error_code &error,
				  const unsigned int channel,
				  session_type &theSession)
{
	if (!error) {
		cout << "The test channel (#" << channel << ") was closed." << endl;
	} else {
		cerr << "Failed to remove the channel." << endl;
	}
	beep::shutdown_session(theSession);
}

static void
on_got_data(const boost::system::error_code &error,
			const beep::message &msg,
			const unsigned int channel,
			session_type &theSession)
{
	using boost::bind;
	using boost::ref;
	if (!error) {
		cout << "The initiator got " << msg.payload_size()
			 << " bytes of application data on channel " << channel
			 << "!" << endl;
		cout << "Contents:\n";// << a_global_buffer << endl;
		string line;
		istringstream strm(msg.content());
		while (getline(strm, line)) {
			cout << line << endl;
		}
	} else {
		cerr << "Error receiving application data: " << error << "\n";
	}
	theSession.async_close_channel(channel, beep::reply_code::success,
								   bind(on_channel_closed, _1, _2,
										ref(theSession)));
}

static void
on_channel_created(const boost::system::error_code &error,
				   const unsigned int channel,
				   session_type &/*mySession*/)
{
	if (!error) {
		cout << "The test channel (#" << channel
			 << ") was accepted and is ready!" << endl;
	} else { 
		cerr << "Failed to create the channel: " << error << endl;
	}
}

static void
on_network_is_ready(const boost::system::error_code &error,
					const beep::identifier &id,
					session_type &/*theSession*/)
{
	if (error) {
		cerr << "The " << id << " connection failed on: " << error << endl;
		/// \todo close the session
	}
}

static void
on_session_is_ready(const boost::system::error_code &error,
					session_type &theSession)
{
	using boost::bind;
	using boost::ref;
	if (!error) {
		vector<string> supported_profiles;
		theSession.available_profiles(std::back_inserter(supported_profiles));

		if (supported_profiles.empty()) {
			cerr << "The listening session does not support any profiles!\n";
			/// \todo close the session.
			return;
		}
		if (const int chNum =
			theSession.async_add_channel(supported_profiles.front(),
										 bind(on_channel_created, _1, _2,
											  ref(theSession)))) {
			// associate a handler for new data on this channel
			theSession.async_read(chNum, bind(on_got_data, _1, _2, _3, ref(theSession)));
			cout << "Requested a new channel (#" << chNum << ") in session "
				 << theSession.id()
				 << " with profile '"
				 << supported_profiles.front() << "'." << endl;
		}
	} else {
		cerr << "The BEEP session was not initialized.\n";
	}
}

int
main(int /*argc*/, char **/*argv*/)
{
	using boost::ref;
	using boost::bind;
	using namespace boost::asio;

	try {
		io_service service;
		solo_tcp_initiator transport(service);
		session_type client(transport);

		transport.install_network_handler(bind(on_network_is_ready, _1, _2, ref(client)));
		transport.set_endpoint(ip::tcp::endpoint(ip::address::from_string("127.0.0.1"),
												 12345));
		client.install_session_handler(bind(on_session_is_ready, _1, ref(client)));
		service.run();
	} catch (const std::exception &ex) {
		cerr << "Fatal Error: " << ex.what() << endl;
	}
	return 0;
}
