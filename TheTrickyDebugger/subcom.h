#pragma once
#include "SuperVector.h"
#include <unordered_map>
#include <unordered_set>

namespace subcom
{
	static const char *capables[] = { "SERVER", "MESSAGE", "PRIVATE", "RELIABLE", "QUALITATIVE", "FAST", "VERY", "PRO", "EFFECTIVE", "ORIGINAL" };

	extern SuperVector<std::tuple<char, std::string>> server_collector;
	
	extern std::string message;
	extern std::unordered_map<char, std::string> servers;
	extern std::unordered_set<char> whoisonline;

	extern char visible_server_address;
	extern unsigned char server_address;

	extern bool enable;
	void ProLoop();
	void StartProc();
}