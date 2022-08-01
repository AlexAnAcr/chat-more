#include "subcom.h"

#include <vector>
#include <unordered_set>

#include "mo.h"

namespace subcom
{
	SuperVector<std::tuple<char, std::string>> server_collector;

	std::string message;
	std::unordered_map<char, std::string> servers;
	std::unordered_set<char> whoisonline;

	bool enable = true;

	char visible_server_address;
	unsigned char server_address = 0;

	void ProLoop() {
		unsigned char cldata = 10, tcp_indexes_sender = 0;
		while (enable) {
			Sleep(250);
			if (message.length())
				mo::moconnector->SendUDP(mo::moconnector->Broadcast, message.c_str(), message.size(), visible_server_address, 1, false);
			
			if (!tcp_indexes_sender) {
				tcp_indexes_sender = 4;
				mo::moconnector->SendTCPSpecial(mo::moconnector->ReceiveSpecilalTCP_Indexes); //Get clients online
			} else
				--tcp_indexes_sender;

			if (!cldata) {
				cldata = 10;
				servers.clear();
			} else
				--cldata;

			for (auto &item : server_collector.pop_all()) {
				servers[std::get<0>(item)] = std::get<1>(item);
			}
		}
	}

	void StartProc() {
		auto in_dtype = mo::moconnector->Broadcast;
		unsigned short bufflen = 16, bufflen_max = 0;
		char unicast_addr, u4bits, *buffer = new char[bufflen];
		while (enable && mo::moconnector->ReceiveTCP(in_dtype, unicast_addr, u4bits, buffer, bufflen)) {
			if (bufflen > bufflen_max) bufflen_max = bufflen;
			if (in_dtype == mo::moconnector->ReceiveSpecilalTCP_Indexes) {
				whoisonline.clear();
				for (char *ch = buffer, *ec = buffer + bufflen; ch != ec; ++ch) {
					whoisonline.insert(*ch);
				}
			}
		}
		delete[] buffer;
	}
}