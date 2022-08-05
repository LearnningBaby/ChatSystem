#pragma once
#pragma warning(disable:4996)
#include<string>
#include<mutex>
#include<Winsock2.h>
#include<iostream>
#include"ChatMsg.h"

#pragma comment(lib, "ws2_32.lib")

class TcpSvr
{
private:
	TcpSvr();
	TcpSvr(const TcpSvr& p);
public:
	static TcpSvr* getinstance();

	int Send(std::string& msg);

	int Recv(std::string* msg);

private:
	int ConnetcToSvr();

private:
	static TcpSvr* instance_;
	SOCKET sockfd_;
};