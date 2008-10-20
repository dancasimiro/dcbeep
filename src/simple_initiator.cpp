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
		service.run();
	} catch (const std::exception &ex) {
		cerr << "Fatal Error: " << ex.what() << endl;
	}
	return 0;
}
