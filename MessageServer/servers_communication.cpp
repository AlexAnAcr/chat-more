#include "servers_communication.h"

namespace scomm
{
	std::unordered_set<char> saddresses;

	bool stat_detailmode = false;
	ThreadSafePacketVector<std::tuple<char, char, std::string>> stat_serversusers;
	ThreadSafePacketVector<std::tuple<char, std::string>> stat_servers;
}