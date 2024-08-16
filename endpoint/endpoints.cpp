#include <iostream>
#include "endpoint.h"
#include <boost/asio.hpp>
using namespace boost;

int client_end_point() {
	std::string raw_ip_address = "172.1.1.1";
	unsigned short port_num = 3333;
	boost::system::error_code ec;
	asio::ip::address ip_address = asio::ip::address::from_string(raw_ip_address,ec);
	if (ec.value() != 0) {
		std::cout << "Failed to parse the ip address. Error code" << ec.value() << ".Message is" << ec.message();
		return ec.value();
	}
	asio::ip::tcp::endpoint(ip_address, port_num);


}	