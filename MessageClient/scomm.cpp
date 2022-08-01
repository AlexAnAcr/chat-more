#include "scomm.h"

#include <vector>
#include <iostream>
#include <algorithm> 

#include "mconn.h"

static std::vector<std::string> servIdsWords = { "SERVER", "MESSAGE", "PRIVATE", "RELIABLE", "QUALITATIVE", "FAST", "VERY", "PRO", "EFFECTIVE", "ORIGINAL", "" };
static std::vector<std::vector<unsigned short>> wish_weight;

static unsigned char CalcScore_finder(const std::string& what) {
	for (unsigned char i = 0; i != 10; ++i) {
		if (servIdsWords[i] == what) {
			return i;
		}
	}
	return 10;
}

static inline void ltrim(std::string &s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
}

// trim from end (in place)
static inline void rtrim(std::string &s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string &s) {
	ltrim(s);
	rtrim(s);
}

static unsigned short CalcScore(const std::string& wishs) {
	std::string prew = "", next = "";
	std::stringstream input_stringstream(wishs);

	unsigned short score = 0;
	while (std::getline(input_stringstream, next, ' ')) {
		trim(next);
		score += wish_weight[CalcScore_finder(prew)][CalcScore_finder(next)];
		prew = next;
	}
	score += wish_weight[CalcScore_finder(prew)][CalcScore_finder(next)];
	return score;
}

namespace scomm
{
	ThreadSafePacketVector<std::tuple<char, std::string>> stat_servers;

	static std::string message;
	std::unordered_map<char, std::string> serverlist;
	bool enable = true;

	volatile short selected_server = -1;

	void StatProcLoop() {
		srand(time(NULL));
		{
			wish_weight.clear();
			wish_weight.reserve(11);

			std::vector<unsigned short> internalvec;
			internalvec.resize(11, 0);

			for (unsigned short i = 0; i != 11; ++i) {
				for (auto &item : internalvec) {
					item = rand() % 20 + 1;
				}
				wish_weight.emplace_back(internalvec);
			}
			wish_weight[10][10] = 0; //Set zero-weight to empty->empty combination. Needs to prohibit valuing of non-allowed words.

			std::string first = "";
			std::string msg = "";
			
			for (unsigned char i = 0; i != 5; ++i) {
				auto &vec = wish_weight[CalcScore_finder(first)];
				first = servIdsWords[std::max_element(vec.begin(), vec.end() - 1) - vec.begin()];;
				msg += ' ' + first;
			}
			message = msg.substr(1);
		}

		bool floating = true;;

		unsigned char cldata = 3;
		while (enable) {
			Sleep(250);
			if (selected_server != -1)
				mo::moconnector->SendUDP(mo::moconnector->Broadcast, message.c_str(), message.size(), selected_server, 1, false);

			if (!cldata) {
				cldata = 5;
				serverlist.clear();
			} else
				--cldata;

			for (auto &item : stat_servers.pop_all()) {
				serverlist[std::get<0>(item)] = std::get<1>(item);
			}


			if (floating) {
				short max_score = 0, index = -1;
				for (auto &item : serverlist) {
					short score = CalcScore(item.second);
					if (score > max_score) {
						max_score = score;
						index = static_cast<unsigned char>(item.first);
					}
				}

				if (index != -1) {
					selected_server = index;
					floating = false;
					//Sending data to console
					std::cout << "Message server found!" << std::endl;
					std::cout << "Press any key to continue..." << std::endl;
				}
			}
		}
	}

	void StartTCProc() {
		auto in_dtype = mo::moconnector->Broadcast;
		unsigned short bufflen = 16;
		char unicast_addr, u4bits, *buffer = new char[bufflen];
		while (scomm::enable && mo::moconnector->ReceiveTCP(in_dtype, unicast_addr, u4bits, buffer, bufflen));
		delete[] buffer;
	}
}