// SyncClient.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include <boost/asio.hpp>
#include <iostream>
using namespace std;
using namespace boost::asio::ip;
const int MAX_LENGTH = 1024;
int main()
{
	try
	{
		//创建io上下文
		boost::asio::io_context ioc;
		//构建endpoint
		tcp::endpoint remote_ep(address::from_string("127.0.0.1"), 10086);
		tcp::socket sock(ioc);
		boost::system::error_code error = boost::asio::error::host_not_found;
		sock.connect(remote_ep, error);
		if (error)
		{
			cout << "Connect failed,code is " << error.value() <<" error message is " << error.message();
			return 0;
		}

		std::cout << "Enter message: ";
		char request[MAX_LENGTH];
		std::cin.getline(request, MAX_LENGTH);
		size_t request_length = strlen(request);
		boost::asio::write(sock, boost::asio::buffer(request, request_length));

		char reply[MAX_LENGTH];
		size_t reply_length = boost::asio::read(sock, boost::asio::buffer(reply, request_length));
		std::cout << "reply is: ";
		std::cout.write(reply, reply_length);
		std::cout << "\n";

	}
	catch (const std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << endl;
	}
	return 0;
}

