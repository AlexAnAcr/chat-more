#pragma once
#include "ThreadSafePacketVector.h"
#include <unordered_map>

namespace scomm
{
	extern ThreadSafePacketVector<std::tuple<char, std::string>> stat_servers;
	
	extern std::unordered_map<char, std::string> serverlist;

	extern volatile short selected_server;

	extern bool enable;
	void StatProcLoop();
	void StartTCProc();
}