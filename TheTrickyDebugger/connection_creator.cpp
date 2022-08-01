#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <ws2tcpip.h>

//Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")

#include "client.h"
#include "mo.h"

#include <string>

//TODO: fix missing

void comain()
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	mo::moconnector = new Client::Processor<SOCKET, INVALID_SOCKET, SOCKET_ERROR, sockaddr>(functions);

	if (!mo::moconnector->Initialise(mo::ip.c_str(), mo::port)) {
		throw std::exception("Can't connect to connection server!");
	}
	mo::address = mo::moconnector->Getaddress();
}
