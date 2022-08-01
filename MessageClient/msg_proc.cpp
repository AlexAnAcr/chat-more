#include "msg_proc.h"
#include <chrono>

namespace msg
{
	std::unordered_map<std::string, interlocutor> archive;

	static std::mutex wait_mutex;
	std::condition_variable wait;

	struct saved_requests saved_reqs;

	//0-4bits, 1-data
	//specific_type can not be neither 1 nor 2
	std::tuple<char, std::string> RetriveResultByType(char specific_type) {
		saved_reqs.request_packet(specific_type);
		saved_reqs.request_packet(2);
		saved_reqs.request_packet(1);

		std::unique_lock lock(wait_mutex);

		std::vector<std::tuple<char, std::string>> result;

		while (true) {
			result = std::move(saved_reqs.fetch_packets_if_exists(specific_type));
			if (result.size() != 0)
				break;

			result = std::move(saved_reqs.fetch_packets_if_exists(2));
			if (result.size() != 0)
				break;

			result = std::move(saved_reqs.fetch_packets_if_exists(1));
			if (result.size() != 0)
				break;
			
			wait.wait_for(lock, std::chrono::milliseconds(100));
		}

		saved_reqs.unrequest_packet(1);
		saved_reqs.unrequest_packet(2);
		saved_reqs.unrequest_packet(specific_type);

		return *(--result.end());
	}
}