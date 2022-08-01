#pragma once
#include "client.h"
#include <winsock2.h>
#include "SafeQ.h"

namespace mo
{
	extern Client::Processor<SOCKET> *moconnector;
	extern std::string ip;
	extern unsigned short port;
	extern char address;

	void ReceiveLoop();

	extern SafeQ sender;
	void SendLoop();
}