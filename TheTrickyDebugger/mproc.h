#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>

namespace msg
{
	struct message {
		std::string message;
		std::chrono::system_clock::time_point datetime;
	};

	struct interlocutor {
		bool online;
		std::vector<message> incoming, outcoming;
	};

	//login -> data;
	extern std::unordered_map<std::string, interlocutor> archive;
}