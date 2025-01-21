#pragma once

#include <queue>
#include <thread>
#include "CSession.h"
#include <queue>
#include <map>
#include <functional>
#include "const.h"
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>

typedef  function<void(shared_ptr<CSession>, const short& msg_id, const string& msg_data)> FunCallBack;
class LogicSystem
{
public:
	~LogicSystem();
	void PostMsgToQue(shared_ptr < LogicNode> msg);
	static LogicSystem& GetInstance();
private:
	LogicSystem();
	void DealMsg();
	void RegisterCallBacks();
	void HelloWordCallBack(shared_ptr<CSession>, const short& msg_id, const string& msg_data);
	std::thread _worker_thread;
	std::queue<shared_ptr<LogicNode>> _msg_que;
	std::mutex _mutex;
	std::condition_variable _consume;
	bool _b_stop;
	std::map<short, FunCallBack> _fun_callbacks;
};



