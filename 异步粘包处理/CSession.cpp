#include "CSession.h"
#include "CServer.h"
#include <iostream>
#include <iomanip>
CSession::CSession(boost::asio::io_context& io_context, CServer* server) :
	_socket(io_context), _server(server), _b_close(false), _b_head_parse(false) {
	boost::uuids::uuid  a_uuid = boost::uuids::random_generator()();
	_uuid = boost::uuids::to_string(a_uuid);
	_recv_head_node = make_shared<MsgNode>(HEAD_LENGTH);
}

CSession::~CSession() {
	cout << "~CSession " << _uuid << " destruct" << endl;
}

tcp::socket& CSession::GetSocket() {
	return _socket;
}

std::string& CSession::GetUuid() {
	return _uuid;
}

void CSession::Start() {
	::memset(_data, 0, MAX_LENGTH);
	_socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH), std::bind(&CSession::HandleRead, this,
		std::placeholders::_1, std::placeholders::_2, SharedSelf()));
}

void CSession::Close() {
	_socket.close();
	_b_close = true;
}

std::shared_ptr<CSession>CSession::SharedSelf() {
	return shared_from_this();
}

void CSession::Send(char* msg, int max_length) {
	bool pending = false;
	std::lock_guard<std::mutex> lock(_send_lock);
	if (_send_que.size() > 0) {
		pending = true;
	}
	_send_que.push(make_shared<MsgNode>(msg, max_length));
	if (pending) {
		return;
	}
	auto& msgnode = _send_que.front();
	boost::asio::async_write(_socket, boost::asio::buffer(msgnode->_data, msgnode->_total_len),
		std::bind(&CSession::HandleWrite, this, std::placeholders::_1, SharedSelf()));
}

void CSession::HandleWrite(const boost::system::error_code& error, shared_ptr<CSession> _self_shared) {
	if (!error) {
		std::lock_guard<std::mutex> lock(_send_lock);
		cout << "send data " << _send_que.front()->_data + HEAD_LENGTH << endl;
		_send_que.pop();
		if (!_send_que.empty()) {
			auto& msgnode = _send_que.front();
			boost::asio::async_write(_socket, boost::asio::buffer(msgnode->_data, msgnode->_total_len),
				std::bind(&CSession::HandleWrite, this, std::placeholders::_1, _self_shared));
		}
	}
	else {
		std::cout << "handle write failed, error is " << error.what() << endl;
		Close();
		_server->ClearSession(_uuid);
	}
}


void CSession::PrintRecvData(char* data, int length) {
	stringstream ss;
	string result = "0x";
	for (int i = 0; i < length; i++) {
		string hexstr;
		ss << hex << std::setw(2) << std::setfill('0') << int(data[i]) << endl;
		ss >> hexstr;
		result += hexstr;
	}
	std::cout << "receive raw data is : " << result << endl;;
}

void CSession::HandleRead(const boost::system::error_code& error, size_t  bytes_transfered, shared_ptr<CSession> _self_shared) {
	if (!error) {
		PrintRecvData(_data, bytes_transfered);
		std::chrono::milliseconds dura(2000);
		std::this_thread::sleep_for(dura);
		//已经移动的字符长度
		int copy_len = 0;
		while (bytes_transfered > 0) {
			if (!_b_head_parse) {
				//收到的数据大小不足头部大小
				if (bytes_transfered + _recv_head_node->_cur_len < HEAD_LENGTH) {
					memcpy(_recv_head_node->_data + _recv_head_node->_cur_len, _data + copy_len, bytes_transfered);
					_recv_head_node->_cur_len += bytes_transfered;
					::memset(_data, 0, MAX_LENGTH);
					_socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
						std::bind(&CSession::HandleRead, this, std::placeholders::_1, std::placeholders::_2, _self_shared));
					return;
				}
				//收到的数据比头部多
				//头部剩余未复制的长度
				int head_remain = HEAD_LENGTH - _recv_head_node->_cur_len;
				memcpy(_recv_head_node->_data + _recv_head_node->_cur_len, _data + copy_len, head_remain);
				//更新已经处理的data长度和剩余的长度
				copy_len += head_remain;
				bytes_transfered -= head_remain;
				//获取头部的数据
				short data_len = 0;
				memcpy(&data_len, _recv_head_node->_data, HEAD_LENGTH);
				cout << "data len is: " << data_len << endl;
				//头部长度非法
				if (data_len > MAX_LENGTH) {
					std::cout << "invalid data length is " << data_len << endl;
					_server->ClearSession(_uuid);
					return;
				}
				_recv_msg_node = make_shared<MsgNode>(data_len);

				//消息长度小于头部规定的长度，说明数据还有一些未收全，则先将收到的这部分放入到节点中
				if (bytes_transfered < data_len) {
					memcpy(_recv_msg_node->_data + _recv_msg_node->_cur_len, _data + copy_len, bytes_transfered);
					_recv_msg_node->_cur_len += bytes_transfered;
					::memset(_data, 0, MAX_LENGTH);
					_socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
						std::bind(&CSession::HandleRead, this, std::placeholders::_1, std::placeholders::_2, _self_shared));
					//头部处理完成
					_b_head_parse = true;
					return;
				}

				memcpy(_recv_msg_node->_data + _recv_msg_node->_cur_len, _data + copy_len, data_len);
				_recv_msg_node->_cur_len += data_len;
				copy_len += data_len;
				bytes_transfered -= data_len;
				_recv_msg_node->_data[_recv_msg_node->_total_len] = '\0';
				cout << "receive data is " << _recv_msg_node->_data << endl;
				//此处调用Send发送测试
				Send(_recv_msg_node->_data, _recv_msg_node->_total_len);
				//继续轮询剩余未处理数据
				_b_head_parse = false;
				_recv_head_node->clear();
				if (bytes_transfered <= 0) {
					::memset(_data, 0, MAX_LENGTH);
					_socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
						std::bind(&CSession::HandleRead, this, std::placeholders::_1, std::placeholders::_2, _self_shared));
					return;
				}
				continue;
			}

			//已经处理完头部，处理上次未接收完的消息数据
			//接受的数据任不足剩余未处理的
			int remain_msg = _recv_msg_node->_total_len - _recv_msg_node->_cur_len;
			if (bytes_transfered < remain_msg) {
				memcpy(_recv_msg_node->_data + _recv_msg_node->_cur_len, _data + copy_len, bytes_transfered);
				_recv_msg_node->_cur_len += bytes_transfered;
				::memset(_data, 0, MAX_LENGTH);
				_socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
					std::bind(&CSession::HandleRead, this, std::placeholders::_1, std::placeholders::_2, _self_shared));
				return;
			}
			memcpy(_recv_msg_node->_data + _recv_msg_node->_cur_len, _data + copy_len, remain_msg);
			_recv_msg_node->_cur_len += remain_msg;
			bytes_transfered -= remain_msg;
			copy_len += remain_msg;
			_recv_msg_node->_data[_recv_msg_node->_total_len] = '\0';
			cout << "receive data is " << _recv_msg_node->_data << endl;
			//此处可以调用Send发送测试
			Send(_recv_msg_node->_data, _recv_msg_node->_total_len);
			//继续轮询剩余未处理数据
			_b_head_parse = false;
			_recv_head_node->clear();
			if (bytes_transfered <= 0) {
				::memset(_data, 0, MAX_LENGTH);
				_socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
					std::bind(&CSession::HandleRead, this, std::placeholders::_1, std::placeholders::_2, _self_shared));
				return;
			}
			continue;
		}
	}
	else {
		std::cout << "handle read failed, error is " << error.what() << endl;
		Close();
		_server->ClearSession(_uuid);
	}
}