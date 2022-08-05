#include "pch.h"
#include "MsgQueue.h"

std::mutex g_lock_;

MsgQueue::MsgQueue()
{
	v_msg_.resize(20);
}

MsgQueue* MsgQueue::instance_ = NULL;

MsgQueue* MsgQueue::GetInstance()
{
	if (NULL == instance_)
	{
		g_lock_.lock();
		if (NULL == instance_)
		{
			instance_ = new MsgQueue();
		}
		g_lock_.unlock();
	}
	return instance_;
}

void MsgQueue::Push(int msg_type, const std::string& msg)
{
	g_lock_.lock();
	v_msg_[msg_type].push(msg);
	g_lock_.unlock();
}

void MsgQueue::Pop(int msg_type, std::string* msg)
{
	//1.判断相应的消息类型到底有没有
	while (1)
	{
		if (v_msg_[msg_type].empty())
		{
			Sleep(1);//毫秒
			continue;
		}

		//2.返回给调用者具体消息
		g_lock_.lock();
		*msg = v_msg_[msg_type].front();
		v_msg_[msg_type].pop();
		g_lock_.unlock();
		break;
	}
}
