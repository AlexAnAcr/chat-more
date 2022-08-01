#include "mconn.h"
#include <set>

#include "scomm.h"
#include "msg_proc.h"

static std::tm StructToYMDHMS(uint32_t bytes) {
	std::tm datetime;

	datetime.tm_year = (bytes >> 26) + 122;
	datetime.tm_mon = (bytes >> 22) & 0xF;
	datetime.tm_mday = ((bytes >> 17) & 0x1F) + 1;
	int hrs = bytes & 0x1FFFF;
	datetime.tm_hour = hrs / 3600;
	hrs %= 3600;
	datetime.tm_min = hrs / 60;
	datetime.tm_sec = hrs % 60;

	return datetime;
}

namespace mo
{
	Client::Processor<SOCKET, INVALID_SOCKET, SOCKET_ERROR, sockaddr> *moconnector;
	std::string ip;
	unsigned short port;
	char address;

	void ReceiveLoop() {
		auto in_dtype = moconnector->Broadcast;
		char in_type, in_4bits, in_addr_broadcast;
		char *in_buffer = new char[1421];
		unsigned short in_buffersize;

		uint32_t minid = 0;
		std::set<uint32_t> uids;

		while (scomm::enable) {
			moconnector->ReceiveUDP(in_dtype, in_type, in_4bits, in_buffer, in_buffersize, in_addr_broadcast);
			if (in_dtype == moconnector->Unicast) {
				if (in_buffersize > 3) {
					if (in_4bits) {
						moconnector->SendUDP(moconnector->Unicast, in_buffer, 4, address, 0, false, in_type);
						if (!uids.contains(*reinterpret_cast<uint32_t *>(in_buffer)) && minid < *reinterpret_cast<uint32_t *>(in_buffer)) {
							uids.insert(*reinterpret_cast<uint32_t *>(in_buffer));
							if (uids.size() == 257) {
								minid = *uids.cbegin();
								uids.erase(uids.cbegin());
							}
						
							if (in_4bits == 7) {
								if (in_buffersize > 10 && in_buffer[8] != 0 && static_cast<unsigned char>(in_buffer[8]) + 9 < in_buffersize) {
									auto datetime = StructToYMDHMS(*reinterpret_cast<uint32_t *>(in_buffer + 4));
									auto login = std::string(in_buffer + 9, in_buffer[8]);
									auto message = std::string(in_buffer + 9 + in_buffer[8], in_buffersize - 9 - in_buffer[8]);
									if (!msg::archive.contains(login)) {
										msg::archive.insert(std::make_pair(login, msg::interlocutor()));
									}
									msg::archive[login].incoming.emplace_back(message, datetime);
								}
							} else if (in_4bits == 8) {
								if (in_buffersize > 10 && in_buffer[8] != 0 && static_cast<unsigned char>(in_buffer[8]) + 9 < in_buffersize) {
									auto datetime = StructToYMDHMS(*reinterpret_cast<uint32_t *>(in_buffer + 4));
									auto login = std::string(in_buffer + 9, in_buffer[8]);
									auto message = std::string(in_buffer + 9 + in_buffer[8], in_buffersize - 9 - in_buffer[8]);
									if (!msg::archive.contains(login)) {
										msg::archive.insert(std::make_pair(login, msg::interlocutor()));
									}
									msg::archive[login].incoming.emplace_back(message, datetime, 1);
									if (!msg::archive.contains("@all")) {
										msg::archive.insert(std::make_pair("@all", msg::interlocutor()));
									}
									msg::archive["@all"].incoming.emplace_back(message, datetime, 1);
								}
							} else if (in_4bits == 9) {
								if (in_buffersize > 10 && in_buffer[8] != 0 && static_cast<unsigned char>(in_buffer[8]) + 9 < in_buffersize) {
									auto datetime = StructToYMDHMS(*reinterpret_cast<uint32_t *>(in_buffer + 4));
									auto login = std::string(in_buffer + 9, in_buffer[8]);
									auto message = std::string(in_buffer + 9 + in_buffer[8], in_buffersize - 9 - in_buffer[8]);
									if (!msg::archive.contains(login)) {
										msg::archive.insert(std::make_pair(login, msg::interlocutor()));
									}
									msg::archive[login].incoming.emplace_back(message, datetime, 2);
									if (!msg::archive.contains("@online")) {
										msg::archive.insert(std::make_pair("@online", msg::interlocutor()));
									}
									msg::archive["@online"].incoming.emplace_back(message, datetime, 2);
								}
							} else if (msg::saved_reqs.push_packet_if_requested(std::make_tuple(in_4bits, std::string(in_buffer + 4, in_buffersize - 4))))
								msg::wait.notify_one();
						}
					} else {
						//Remove pack from sender queue
						sender.erase(*reinterpret_cast<uint32_t *>(in_buffer));
					}
				}
			} else {
				//in_type - sender address
				if (!in_4bits) {
					scomm::stat_servers.push(in_addr_broadcast, std::string(in_buffer, in_buffersize));
				}
			}
		}
	}

	SafeQ sender;

	void SendLoop() {
		while (scomm::enable) {
			//Sending sheduled packkets
			for (auto &pack : sender.get_snapshot()) {
				std::string &data = std::get<3>(pack);
				mo::moconnector->SendUDP(mo::moconnector->Unicast, data.c_str(), data.length(), mo::address, std::get<2>(pack), false, std::get<1>(pack));
			}
			Sleep(250);
		}
	}
}

