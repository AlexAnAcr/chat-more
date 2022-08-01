#pragma once
#include <string>
#include <vector>
#include <unordered_map>

#include "ThreadSafePacketVector.h"

namespace msg
{
	struct message {
		message(const std::string &message, const std::tm &datetime, unsigned char broadcast = 0):msg(message) {
			this->datetime = datetime;
			this->broadcast = broadcast;
		}

		std::string msg;
		std::tm datetime;
		bool readen = false;
		unsigned char broadcast; //0 - unicast; 1 - broadcast to all; 2 - broadcast to onlines
	};
	struct interlocutor {
		std::vector<message> incoming, outcoming;
	};
	//login -> data;
	extern std::unordered_map<std::string, interlocutor> archive;

	//0-4bits, 1-data
	struct saved_requests {
		//0-4bits, 1-data
		bool push_packet_if_requested(const std::tuple<char, std::string> &packet) {
			bool ret;
			mtx.lock();
			if (ret = saved_requests.contains(std::get<0>(packet)))
				saved_requests[std::get<0>(packet)].emplace_back(packet);
			mtx.unlock();
			return ret;
		}

		void request_packet(char type) {
			if (saved_requests.contains(type))
				throw("request_packet call duplicate");
			mtx.lock();
			saved_requests.insert(std::make_pair(type, std::vector<std::tuple<char, std::string>>()));
			mtx.unlock();
		}

		void unrequest_packet(char type) {
			mtx.lock();
			saved_requests.erase(type);
			mtx.unlock();
		}

		std::vector<std::tuple<char, std::string>> fetch_packets_if_exists(char type) {
			std::vector<std::tuple<char, std::string>> returnval;
			mtx.lock();
			if (saved_requests.contains(type)) {
				returnval = std::move(saved_requests[type]);
			}
			mtx.unlock();
			return returnval;
		}
	private:
		//0-4bits, 1-data
		std::unordered_map<char, std::vector<std::tuple<char, std::string>>> saved_requests;
		std::mutex mtx;
	};
	extern struct saved_requests saved_reqs;

	extern std::condition_variable wait;
	//0-4bits, 1-data
	//specific_type can not be neither 1 nor 2
	std::tuple<char, std::string> RetriveResultByType(char specific_type = 0);
	
}