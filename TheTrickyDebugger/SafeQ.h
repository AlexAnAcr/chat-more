#pragma once
#include <unordered_map>
#include <vector>
#include <mutex>
#include <string>
#include <tuple>

class SafeQ {
	std::mutex m_mutex;
	std::unordered_map<uint32_t, std::tuple<char, char, char, std::string>> sender;
public:
	void push(uint32_t key, char sender_addr, char dest_addr, char d4bits, const std::string& value) {
		std::string ready_message(reinterpret_cast<const char*>(&key), sizeof(uint32_t));
		ready_message.append(value);
		m_mutex.lock();
		sender[key] = std::move(std::make_tuple(sender_addr, dest_addr, d4bits, ready_message));
		m_mutex.unlock();
	}

	void erase(uint32_t key) {
		m_mutex.lock();
		sender.erase(key);
		m_mutex.unlock();
	}
	
	void clear() {
		m_mutex.lock();
		sender.clear();
		m_mutex.unlock();
	}

	//sender_addr, dest_addr, 4bits, data
	std::vector<std::tuple<char, char, char, std::string>> get_snapshot() {
		std::vector<std::tuple<char, char, char, std::string>> res;
		res.reserve(sender.size());
		m_mutex.lock();
		for (auto& item : sender) {
			res.emplace_back(item.second);
		}
		m_mutex.unlock();
		return res;
	}
};