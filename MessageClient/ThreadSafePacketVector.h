#pragma once
#include <vector>
#include <mutex>

template<typename T>
class ThreadSafePacketVector {
	std::mutex m_mutex;
	std::vector<T> m_vector;
public:
	template<class... Args>
	void push(Args&&... elem) {
		m_mutex.lock();
		m_vector.emplace_back(elem...);
		m_mutex.unlock();
	}
	std::vector<T> pop_all() {
		m_mutex.lock();
		std::vector<T> res = std::move(m_vector);
		m_mutex.unlock();
		return res;
	}
};