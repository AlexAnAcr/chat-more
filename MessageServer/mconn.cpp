#include "mconn.h"
#include "clients_basis.h"
#include "servers_communication.h"

namespace mo
{
	Client::Processor<SOCKET, INVALID_SOCKET, SOCKET_ERROR, sockaddr> *moconnector;
	const char *ip;
	unsigned short port;
	char address;

	void ReceiveLoop() {
		auto in_dtype = moconnector->Broadcast;
		char in_type, in_4bits, in_addr_broadcast;
		char *in_buffer = new char[1421]; //UDP packet max allowed size
		unsigned short in_buffersize;

		while (true) {
			moconnector->ReceiveUDP(in_dtype, in_type, in_4bits, in_buffer, in_buffersize, in_addr_broadcast);
			if (in_dtype == moconnector->Unicast) {
				//in_type - sender address
				//in_4bits != 0 - packet is request, in_4bits == 0 packet is technical
				if (in_buffersize > 3) {
					if (in_4bits) {
						//Returning message id (int32) as confirmation
						moconnector->SendUDP(moconnector->Unicast, in_buffer, 4, address, 0, false, in_type);
						clibs::requests.push(in_type, *reinterpret_cast<uint32_t *>(in_buffer), in_4bits, std::string(in_buffer + 4, in_buffersize - 4));
						clibs::wait.notify_one();
					} else {
						//Remove pack from sender queue
						clibs::sender.erase(*reinterpret_cast<uint32_t *>(in_buffer));
					}
				}
			} else {
				//in_type - sender address
				if (in_4bits == 1) {
					if (!scomm::stat_detailmode) {
						scomm::stat_serversusers.push(in_addr_broadcast, in_type, "");
					} else {
						scomm::stat_serversusers.push(in_addr_broadcast, in_type, std::string(in_buffer, in_buffersize));
					}
				} else if (in_4bits == 0) {
					if (!scomm::stat_detailmode) {
						scomm::stat_servers.push(in_addr_broadcast, "");
					} else {
						scomm::stat_servers.push(in_addr_broadcast, std::string(in_buffer, in_buffersize));
					}
				}
			}
		}
	}
}

