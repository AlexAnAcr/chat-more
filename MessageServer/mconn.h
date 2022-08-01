#pragma once
#include "connection_client.h"
#include <winsock2.h>
#include <tuple>
#include "ThreadSafePacketVector.h"

namespace mo
{
	extern Client::Processor<SOCKET, INVALID_SOCKET, SOCKET_ERROR, sockaddr> *moconnector;
	extern const char *ip;
	extern unsigned short port;
	extern char address;

	void ReceiveLoop();
}