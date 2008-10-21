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

char a_global_buffer[4096];

void
on_got_data(const boost::system::error_code &error,
			std::size_t bytes_transferred)
{
	if (!error || error == boost::asio::error::message_size) {
		cout << "The initiator got " << bytes_transferred
			 << " bytes of application data!" << endl;
		a_global_buffer[bytes_transferred] = '\0';
		cout << "Contents:\n" << a_global_buffer << endl;
	} else {
		cerr << "Error receiving application data.\n";
	}
}



int
main(int argc, char **argv)
{
	typedef beep::tcptl                                     transport_layer;
	typedef beep::basic_session<transport_layer>            session;
	typedef beep::basic_initiator<session>                  initiator;
	typedef beep::basic_channel<session>                    channel;

	try {
		io_service service;
		transport_layer tl(service);
		initiator client(tl);
		client.connect(ip::tcp::endpoint(ip::address::from_string("127.0.0.1"),
										 12345));
		channel myChannel(client.next_layer());
		shared_ptr<beep::profile> pp(new test_profile);
		myChannel.set_profile(pp);
		client.next_layer().add(myChannel);
		client.next_layer().connection().async_read(myChannel,
													buffer(a_global_buffer),
													on_got_data);
		service.run();
	} catch (const std::exception &ex) {
		cerr << "Fatal Error: " << ex.what() << endl;
	}
	return 0;
}
