#include "mo.h"
#include "subcom.h"

namespace mo
{
	Client::Processor<SOCKET> *moconnector;
	std::string ip;
	unsigned short port;
	char address;

	void ReceiveLoop() {
		auto in_dtype = moconnector->Broadcast;
		char in_type, in_4bits, in_addr_broadcast;
		char *in_buffer = new char[1421];
		unsigned short in_buffersize;

		while (subcom::enable) {
			moconnector->ReceiveUDP(in_dtype, in_type, in_4bits, in_buffer, in_buffersize, in_addr_broadcast);
			if (in_dtype == moconnector->Unicast) {
				if (in_buffersize > 3) {
					if (in_4bits) {
						moconnector->SendUDP(moconnector->Unicast, in_buffer, 4, address, 0, false, in_type);
					} else {
						sender.erase(*reinterpret_cast<uint32_t *>(in_buffer));
					}
				}
			} else {
				if (!in_4bits) {
					subcom::server_collector.push(in_addr_broadcast, std::string(in_buffer, in_buffersize));
				}
			}
		}
	}

	SafeQ sender;
	
	void SendLoop() {
		while (subcom::enable) {
			for (auto &pack : sender.get_snapshot()) {
				std::string &data = std::get<3>(pack);
				mo::moconnector->SendUDP(mo::moconnector->Unicast, data.c_str(), data.length(), std::get<0>(pack), std::get<2>(pack), false, std::get<1>(pack));
				if (std::get<2>(pack) != mo::address) {
					sender.erase(*reinterpret_cast<const uint32_t *>(data.c_str()));
				}
			}
			Sleep(50);
		}
	}
}

