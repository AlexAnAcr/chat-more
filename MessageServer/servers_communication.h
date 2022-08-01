#pragma once
#include "ThreadSafePacketVector.h"
#include <unordered_set>

namespace scomm
{
	void ServerIdBroadcasteLoop();

	extern std::unordered_set<char> saddresses;

	extern bool stat_detailmode;
	//client_addr, uses_server, ~wanted_str
	extern ThreadSafePacketVector<std::tuple<char, char, std::string>> stat_serversusers;
	
	//server_addr, serv_str
	extern ThreadSafePacketVector<std::tuple<char, std::string>> stat_servers;
}
