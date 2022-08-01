#pragma once
#include "connection_client.h"
#include <winsock2.h>
#include "SafeQ.h"

namespace mo
{
	extern Client::Processor<SOCKET, INVALID_SOCKET, SOCKET_ERROR, sockaddr> *moconnector;
	extern std::string ip;
	extern unsigned short port;
	extern char address;

	void ReceiveLoop();

	extern SafeQ sender;
	void SendLoop();
}