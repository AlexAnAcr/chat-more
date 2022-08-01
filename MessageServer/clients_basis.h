#pragma once
#include <unordered_map>
#include <string>
#include <mutex>
#include <tuple>
#include "ThreadSafePacketVector.h"
#include "SafeQ.h"

namespace clibs
{
	//Physical (connection interface-based address) -> client login <=> online list
	extern std::unordered_map<char, std::string> addr_translation;
	extern std::mutex addr_translation_mutex;

	//0-from, 1-id, 2-4bits, 3-data
	extern ThreadSafePacketVector<std::tuple<char, uint32_t, char, std::string>> requests;

	extern std::condition_variable wait;

	extern SafeQ sender;

	extern void ClientProcessingLoop();
}

