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
		std::cout << "Failed to parse the ip address. Error code" 
			<< ec.value() << ".Message is" << ec.message();
		return ec.value();
	}
	asio::ip::tcp::endpoint(ip_address, port_num);
	return 0;
}	

int server_end_point() {
	unsigned short port_num = 3333;
	asio::ip::address ip_address = asio::ip::address_v6::any();
	asio::ip::tcp::endpoint(ip_address,port_num);
	return 0;
}

int create_tcp_socket() {
	asio::io_context ioc;//创建上下文指定为哪个socket服务
	asio::ip::tcp protocol = asio::ip::tcp::v4();
	asio::ip::tcp::socket sock(ioc);
	boost::system::error_code ec;
	sock.open(protocol, ec);
	if (ec.value()!= 0) {
		std::cout << "Failed to parse the ip address. Error code"
			<< ec.value() << ".Message is" << ec.message();
		return ec.value();
	}
	return 0;
}

int create_acceptor_socket() {
	asio::io_context ios;
	//asio::ip::tcp protocol = asio::ip::tcp::v4();
	//asio::ip::tcp::acceptor acceptor(ios);
	//boost::system::error_code ec;
	//acceptor.open(protocol, ec);
	//if (ec.value() != 0) {
	//	std::cout << "Failed to parse the ip address. Error code"
	//		<< ec.value() << ".Message is" << ec.message();
	//	return ec.value();
	//}
	asio::ip::tcp::acceptor a(ios, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), 3333));
	return 0;
}


int bind_acceptor_socket() {
	unsigned short port_num = 3333;
	asio::io_context ios;
	asio::ip::tcp::endpoint ep(asio::ip::address_v4::any(), port_num);
	asio::ip::tcp::acceptor acceptor(ios, ep.protocol());
	boost::system::error_code ec;
	acceptor.bind(ep,ec);
	if (ec.value() != 0) {
		std::cout << "Failed to parse the ip address. Error code"
			<< ec.value() << ".Message is" << ec.message();
		return ec.value();
	}
	return 0;
}

int connect_to_end() {
	std::string raw_ip_address = "192.168.1.12";
	unsigned short port_num = 3333;
	try
	{
		asio::ip::tcp::endpoint ep(asio::ip::address::from_string(raw_ip_address), port_num);
		asio::io_context ios;
		asio::ip::tcp::socket sock(ios, ep.protocol());
		sock.connect(ep);
	}
	catch (system::system_error& e) {
		std::cout << "Error occured! Error code = " << e.code()
			<< ". Message: " << e.what();
		return e.code().value();
	}
	return 0;
}

int dns_connect_to_end() {
	std::string host = "yolo2024.blog.csdn.net";
	std::string port_num = "3333";
	asio::io_context ios;
	asio::ip::tcp::resolver::query resolver_query(host, port_num, asio::ip::tcp::resolver::query::numeric_service);
	asio::ip::tcp::resolver reslover(ios);
	try
	{
		asio::ip::tcp::resolver::iterator it = reslover.resolve(resolver_query);
		asio::ip::tcp::socket sock(ios);
		asio::connect(sock, it);
	}
	catch (system::system_error& e) {
		std::cout << "Error occured! Error code = " << e.code()
			<< ". Message: " << e.what();
		return e.code().value();
	}
	return 0;
}

int accept_new_connect() {
	const int BACKLOG_SIZE = 30;
	unsigned short port_num = 3333;
	asio::ip::tcp::endpoint ep(asio::ip::address_v4::any(), port_num);
	asio::io_context ios;
	try
	{
		asio::ip::tcp::acceptor acceptor(ios, ep.protocol());
		acceptor.bind(ep);
		acceptor.listen(BACKLOG_SIZE);
		asio::ip::tcp::socket sock(ios);
		acceptor.accept(sock);
	}
	catch (system::system_error& e) {
		std::cout << "Error occured! Error code = " << e.code()
			<< ". Message: " << e.what();
		return e.code().value();
	}
	return 0;
}








