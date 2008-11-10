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
using boost::bind;
using boost::shared_ptr;
using namespace boost::asio;
using namespace boost;

#include "beep.hpp"
#include "connection.hpp"
#include "tcp.hpp"
#include "initiator.hpp"
#include "test_profile.hpp"

typedef beep::tcptl                                     transport_layer;
typedef beep::basic_session<transport_layer>            session;
typedef beep::basic_initiator<session>                  initiator;
typedef beep::basic_channel<session>                    channel;

static initiator *a_global_client;
static channel *a_global_channel;
static char a_global_buffer[4096];

void
on_got_data(const boost::system::error_code &error,
			std::size_t bytes_transferred, int channelNumber)
{
	if (!error || error == boost::asio::error::message_size) {
		cout << "The initiator got " << bytes_transferred
			 << " bytes of application data on channel " << channelNumber
			 << "!" << endl;
		a_global_buffer[bytes_transferred] = '\0';
		cout << "Contents:\n";// << a_global_buffer << endl;
		std::string line;
		std::string contents(a_global_buffer, bytes_transferred);
		std::istringstream strm(contents);
		while (std::getline(strm, line)) {
			cout << line << endl;
		}
		a_global_client->next_layer().connection().async_read(*a_global_channel,
															 buffer(a_global_buffer),
															 on_got_data);
	} else {
		cerr << "Error receiving application data.\n";
	}
}

void
on_channel_was_added(initiator &client, channel &myChannel)
{
	a_global_client = &client;
	a_global_channel = &myChannel;
	client.next_layer().connection().async_read(myChannel,
												buffer(a_global_buffer),
												on_got_data);
}


void
on_connect(initiator &client, channel &myChannel)
{
	client.next_layer()
		.async_add(myChannel,
				   bind(on_channel_was_added, ref(client), ref(myChannel)));
}

int
main(int argc, char **argv)
{
	try {
		io_service service;
		transport_layer tl(service);
		initiator client(tl);
		channel myChannel(client.next_layer());
		shared_ptr<beep::profile> pp(new test_profile);
		myChannel.set_profile(pp);

		client.async_connect(ip::tcp::endpoint(ip::address::from_string("127.0.0.1"),
											   12345),
							 bind(on_connect, ref(client), ref(myChannel)));
		service.run();
	} catch (const std::exception &ex) {
		cerr << "Fatal Error: " << ex.what() << endl;
	}
	return 0;
}
