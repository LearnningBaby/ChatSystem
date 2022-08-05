#pragma once
#include<string>
#include<vector>
#include<queue>
#include<mutex>

class MsgQueue
{
private:
	MsgQueue();
	~MsgQueue();
public:
	static MsgQueue* GetInstance();

	void Push(int msg_type, const std::string& msg);
	void Pop(int msg_type, std::string* msg);

private:
	static MsgQueue* instance_;
	std::vector<std::queue<std::string>> v_msg_;
};


