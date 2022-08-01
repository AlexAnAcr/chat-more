#include "mconn.h"

#include <time.h>
#include <string>
#include <unordered_map>

#include "servers_communication.h"
#include "clients_basis.h"

static const char *servIdsWords[] = { "SERVER", "MESSAGE", "PRIVATE", "RELIABLE", "QUALITATIVE", "FAST", "VERY", "PRO", "EFFECTIVE", "ORIGINAL" };
//Max words in packet = 32

static const char **server_seq;
static unsigned char server_seq_size;
static std::string server_seq_string;

static void serverSeq_to_string() {
	server_seq_string = server_seq[0];
	for (unsigned char i = 1; i != server_seq_size; ++i)
		server_seq_string = server_seq_string + ' ' + server_seq[i];
}

namespace scomm
{
	void ServerIdBroadcasteLoop() {
		srand(time(NULL));

		server_seq_size = static_cast<unsigned>(rand()) % 31 + 1;
		server_seq = new const char *[server_seq_size];
		for (unsigned char i = 0; i != server_seq_size; ++i) {
			server_seq[i] = servIdsWords[static_cast<unsigned>(rand()) % 10];
		}
		serverSeq_to_string();
		unsigned char spec_tcp_send_timer = 10;
		while (true) {
			mo::moconnector->SendUDP(mo::moconnector->Broadcast, server_seq_string.c_str(), server_seq_string.size(), mo::address, 0, false);
			if (!spec_tcp_send_timer) {
				spec_tcp_send_timer = 10;
				mo::moconnector->SendTCPSpecial(mo::moconnector->ReceiveSpecilalTCP_Indexes); //Get clients online
			} else
				--spec_tcp_send_timer;

			Sleep(500);
			char max_addr = 0;
			unsigned short maxc = 0;
			if (!scomm::stat_detailmode) {
				for (auto user : stat_servers.pop_all()) {
					saddresses.insert(std::get<0>(user));
				}
				std::unordered_map<char, char> usermap;
				for (auto &user : stat_serversusers.pop_all()) {
					usermap[std::get<0>(user)] = std::get<1>(user);
				}
				std::unordered_map<char, unsigned short> serv_counts;
				for (auto &user : usermap) {
					if (serv_counts.contains(user.second))
						++serv_counts[user.second];
					else
						serv_counts[user.second] = 0;
				}
				for (auto &user : serv_counts) {
					saddresses.erase(user.first);
					if (user.second > maxc) {
						maxc = user.second;
						max_addr = user.first;
					}
				}

				//Not implemented
				/*if ((maxc << 1) > clibs::addr_translation.size()) {
					scomm::stat_detailmode = true;
				}*/
			} else {
				
			}
		}
	}
}
