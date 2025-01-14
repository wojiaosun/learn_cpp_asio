#pragma once
#include <boost/asio.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <queue>
#include <mutex>
#include <memory>
#include <iostream>
using namespace std;
#define MAX_LENGTH  1024*2
#define HEAD_LENGTH 2
using boost::asio::ip::tcp;
class CServer;

class MsgNode
{
	friend class CSession;
public:
	//发消息
	MsgNode(char* msg, short max_len) :_total_len(max_len+HEAD_LENGTH),_cur_len(0) {
		_data = new char[_total_len+1]();
		memcpy(_data, &max_len, HEAD_LENGTH);
		memcpy(_data + HEAD_LENGTH, msg, max_len);
		_data[_total_len] = '\0';
	}
	//收消息
	MsgNode(short max_len) :_total_len(max_len), _cur_len(0) {
		_data = new char[_total_len + 1]();
	}

	~MsgNode() {
		delete[] _data;
	}

	void clear() {
		memset(_data, 0, _total_len);
		_cur_len = 0;
	}

private:
	short _cur_len;
	short _total_len;
	char* _data;
};
class CSession :public std::enable_shared_from_this<CSession>
{
public:
	CSession(boost::asio::io_context& io_context, CServer* server);
	~CSession();
	tcp::socket& GetSocket();
	std::string& GetUuid();
	void Start();
	void Close();
	void Send(char* msg, int max_length);
	std::shared_ptr<CSession> SharedSelf();
private:
	void PrintRecvData(char* data, int length);
	void HandleRead(const boost::system::error_code& error, size_t  bytes_transferred, shared_ptr<CSession> _self_shared);
	void HandleWrite(const boost::system::error_code& error, shared_ptr<CSession> _self_shared);
	tcp::socket _socket;
	std::string _uuid;
	char _data[MAX_LENGTH];
	CServer* _server;
	bool _b_close;
	std::queue<shared_ptr<MsgNode> > _send_que;
	std::mutex _send_lock;
	//收到的消息结构
	std::shared_ptr<MsgNode> _recv_msg_node;
	//头部是否解析完
	bool _b_head_parse;
	//收到的头部结构
	std::shared_ptr<MsgNode> _recv_head_node;
};
