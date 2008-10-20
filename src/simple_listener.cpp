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

int
main(int argc, char **argv)
{
	typedef beep::tcptl                                     transport_layer;
	typedef beep::basic_session<transport_layer>            session;
	typedef beep::basic_listener<session>                   listener;
	try {
		io_service service;
		transport_layer tl(service);
		listener server(tl, ip::tcp::endpoint(ip::tcp::v4(), 12345));

		shared_ptr<test_profile> tpp(new test_profile);
		server.install(tpp);
		service.run();
	} catch (const std::exception &ex) {
		cerr << "Fatal Error: " << ex.what() << endl;
	}
	return 0;
}
