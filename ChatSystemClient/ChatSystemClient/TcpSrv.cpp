#include "pch.h"
#include "TcpSvr.h"


std::mutex mt;
TcpSvr* TcpSvr::instance_ = NULL;

TcpSvr::TcpSvr()
{}

TcpSvr* TcpSvr::getinstance()
{
	if (instance_ == NULL)
	{
		mt.lock();
		if (instance_ == NULL)
		{
			instance_ = new TcpSvr();
			if (instance_ != NULL)
			{
				instance_->ConnetcToSvr();
			}
		}
		mt.unlock();
	}
	return instance_;
}

int TcpSvr::Send(std::string& msg)
{
	return send(sockfd_, msg.c_str(), msg.size(), 0);
}

int TcpSvr::Recv(std::string* msg)
{
	char buf[TCP_DATA_MAX_LEN] = { 0 };
	int r_s = recv(sockfd_, buf, sizeof(buf) - 1, 0);
	if (r_s < 0)
	{
		return r_s;
	}
	else if (r_s == 0)
	{
		//重新连接
		//如果没有tcp连接
		exit(1);
	}
	else
	{
		*msg = buf;
	}

	return r_s;
}

int TcpSvr::ConnetcToSvr()
{
	//1.加载套接字库
	WSADATA wsaData;
	int iRet = 0;
	iRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (0 != iRet)
	{
		std::cout << "WSAStartup(MAKEWORD(2, 2), &wsaData)" << std::endl;
		return -1;
	}

	if (2 != LOBYTE(wsaData.wVersion) || 2 != HIBYTE(wsaData.wVersion))
	{
		WSACleanup();
		return -1;
	}

	//2.创建套接字
	sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd_ == INVALID_SOCKET)
	{
		WSACleanup();
		return -1;
	}

	//3.连接服务端
	SOCKADDR_IN srvAddr;
	srvAddr.sin_addr.S_un.S_addr = inet_addr("8.142.191.35");
	srvAddr.sin_family = AF_INET;
	srvAddr.sin_port = htons(TCP_PORT);

	iRet = connect(sockfd_, (struct sockaddr*)&srvAddr, sizeof(srvAddr));
	if (iRet != 0)
	{
		closesocket(sockfd_);
		WSACleanup();

		return -1;
	}

	return 0;
}