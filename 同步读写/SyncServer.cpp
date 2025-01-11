// SyncServer.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <boost/asio.hpp>
#include <set>
#include <memory>
using boost::asio::ip::tcp;
const int max_length = 1024;
typedef std::shared_ptr<tcp::socket> socket_ptr;
std::set<std::shared_ptr<std::thread>> thread_set;
using namespace std;

void session(socket_ptr sock)
{
	try
	{
		for (;;)
		{
			char data[max_length];
			memset(data, '\0', max_length);
			boost::system::error_code error;
			size_t length = sock->read_some(boost::asio::buffer(data, max_length), error);
			if (error == boost::asio::error::eof)
			{
				cout << "connection closed by peer" << endl;
				break;
			}
			else if (error)
			{
				throw boost::system::system_error(error);
			}
			cout << "Receive from " << sock->remote_endpoint().address().to_string() << endl;
			cout << "Receive message is (" << data << ")" << endl;
			//回传信息
			boost::asio::write(*sock, boost::asio::buffer(data, length));
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Exception in thread :" << e.what() << "\n";
	}
}

void server(boost::asio::io_context& io_context, unsigned short port)
{
	cout << "---welcom bz---"<<endl;
	tcp::acceptor a(io_context,tcp::endpoint(tcp::v4(),port));
	for (;;)
	{
		socket_ptr sock(new tcp::socket(io_context));
		a.accept(*sock);
		auto t = std::make_shared<std::thread>(session, sock);
		thread_set.insert(t);
	}
}

int main()
{
	try
	{
		boost::asio::io_context ioc;
		server(ioc, 10086);
		for (auto t:thread_set)
		{
			t->join();
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Exception :" << e.what() << "\n";
	}
	return 0;
}

