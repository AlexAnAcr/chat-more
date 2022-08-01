#include "clients_basis.h"
#include <condition_variable>

#include "mconn.h"
#include "servers_communication.h"


namespace clibs
{
	//PROFILE AREA
	//login -> profile
	static struct clibs_profile {
		clibs_profile(): password(""), address(0) {
		}
		clibs_profile(std::string password, char address): password(password), address(address) {
		}
		void pending_msg(char sender_addr, char d4bits, const std::string &value) {
			pending_msgs.emplace_back(sender_addr, d4bits, value);
		}
		//sender_addr, 4bits, data
		std::vector<std::tuple<char, char, std::string>> get_pendings() {
			return pending_msgs;
		}
		void clear_pendings() {
			pending_msgs.clear();
		}

		char address;
		std::string password;
	private:
		std::vector<std::tuple<char, char, std::string>> pending_msgs;
	};
	static std::unordered_map<std::string, clibs_profile> profiles;

	//SESSION AREA
	//Physical (connection interface-based address) -> client login
	std::mutex addr_translation_mutex;
	std::unordered_map<char, std::string> addr_translation;
	std::unordered_map<char, unsigned char> addr_packanswers_graylist;
	static std::unordered_map<char, std::string> addr_translation_erased;
	static std::unordered_map<char, uint32_t> addr_minid;


	//RECEIVING AREA
	//0-from, 1-id, 2-4bits, 3-data
	ThreadSafePacketVector<std::tuple<char, uint32_t, char, std::string>> requests;

	//SENDING AREA
	SafeQ sender;
	static uint32_t unique_key = 0;

	static std::mutex wait_mutex;
	std::condition_variable wait;
	void ClientProcessingLoop() {
		while (true) {
			std::unique_lock<std::mutex> locker(wait_mutex);
			wait.wait(locker);

			clibs::addr_translation_mutex.lock();

			//Processing incoming requests
			for (auto &rcu : requests.pop_all()) {
				switch (std::get<2>(rcu)) {
					case 1: //Register
						//Logout previous client
						if (addr_translation.contains(std::get<0>(rcu))) {
							if (addr_minid[std::get<0>(rcu)] != std::get<1>(rcu)) {
								//Saving addr_translation_erased
								addr_translation_erased[std::get<0>(rcu)] = addr_translation[std::get<0>(rcu)];
								//Logout
								addr_translation.erase(std::get<0>(rcu));
								addr_minid.erase(std::get<0>(rcu));
							} else { //Probably, duplicate received (register packet from the same client)
								std::string binstr = std::get<3>(rcu);
								if (binstr.length() < 3 || //3+ : 1(char) + 2 (login + password)
									binstr.at(0) == 0 ||
									static_cast<unsigned char>(binstr.at(0)) + 1 >= binstr.length() ||
									binstr.substr(1, static_cast<unsigned char>(binstr.at(0))) != addr_translation[std::get<0>(rcu)]) {
									//Another cient trying to register, logout prewious client
									//Saving addr_translation_erased
									addr_translation_erased[std::get<0>(rcu)] = addr_translation[std::get<0>(rcu)];
									//Logout
									addr_translation.erase(std::get<0>(rcu));
									addr_minid.erase(std::get<0>(rcu));
								} else {
									//Package duplicate received, nothing to do (success answer already sent)
									continue; //Going to next package processing
								}
							}
						}
						{
							std::string binstr = std::get<3>(rcu);
							//3 - min size of reg pack; at(0) - len of login
							if (binstr.length() > 2 && //3+ : 1(char) + 2(login+pass)
								binstr.at(0) != 0 &&
								static_cast<unsigned char>(binstr.at(0)) + 1 < binstr.length()) {

								std::string login = binstr.substr(1, static_cast<unsigned char>(binstr.at(0)));
								if (!profiles.contains(login)) { //No such login
									//Reg
									profiles[login] = clibs_profile(binstr.substr(1 + static_cast<unsigned char>(binstr.at(0))), std::get<0>(rcu));
									scomm::saddresses.erase(std::get<0>(rcu));
									//Login
									addr_translation[std::get<0>(rcu)] = login;
									addr_minid[std::get<0>(rcu)] = std::get<1>(rcu);
									//Clear addr_translation_erased
									addr_translation_erased.erase(std::get<0>(rcu));
									addr_packanswers_graylist.erase(std::get<0>(rcu));
									//Answer
									sender.push(++unique_key, mo::address, std::get<0>(rcu), 2, "REGISTERED");
								} else {
									sender.push(++unique_key, mo::address, std::get<0>(rcu), 1, "USERNAME IS ALREADY TAKEN");
								}
							}
						}
						break;
					case 2: //Unregister
					{
						std::string password = std::get<3>(rcu);
						if (password.length() != 0) {
							if (addr_translation.contains(std::get<0>(rcu))) {
								auto &login = addr_translation[std::get<0>(rcu)];

								if (profiles[login].password == password) {
									//Unreg
									profiles.erase(login);
									//Logout
									addr_translation.erase(std::get<0>(rcu));
									addr_minid.erase(std::get<0>(rcu));
									//Answer
									sender.push(++unique_key, mo::address, std::get<0>(rcu), 2, "UNREGISTERED");
								} else {
									sender.push(++unique_key, mo::address, std::get<0>(rcu), 1, "WRONG PASSWORD");
								}
							} else {
								sender.push(++unique_key, mo::address, std::get<0>(rcu), 1, "NOT LOGGED IN");
							}
						}
					}
					break;
					case 3: //Login
						//Logout previous client if contains
						//"addr_minid" is not needed because re-login will be ended with success is login/pass correct independed of duplicatings
						if (addr_translation.contains(std::get<0>(rcu))) {
							//Saving addr_translation_erased
							addr_translation_erased[std::get<0>(rcu)] = addr_translation[std::get<0>(rcu)];
							//Logout
							addr_translation.erase(std::get<0>(rcu));
							addr_minid.erase(std::get<0>(rcu));
						}
						{
							std::string binstr = std::get<3>(rcu);
							//3 - min size of login pack; at(0) - len of login
							if (binstr.length() > 2 &&
								binstr.at(0) != 0 &&
								static_cast<unsigned char>(binstr.at(0)) + 1 < binstr.length()) {

								std::string login = binstr.substr(1, static_cast<unsigned char>(binstr.at(0)));
								if (profiles.contains(login)) { //Login found
									auto &cp = profiles[login];
									if (cp.password == binstr.substr(1 + static_cast<unsigned char>(binstr.at(0)))) {
										//Remove prew session if not logged out
										if (addr_translation.contains(cp.address) && &profiles[addr_translation[cp.address]] == &cp) {
											addr_translation.erase(cp.address);
											addr_minid.erase(cp.address);
										}
										scomm::saddresses.erase(std::get<0>(rcu));
										//Login
										cp.address = std::get<0>(rcu);
										addr_translation[cp.address] = login;
										addr_minid[cp.address] = std::get<1>(rcu);
										//Clear addr_translation_erased
										addr_translation_erased.erase(std::get<0>(rcu));
										addr_packanswers_graylist.erase(std::get<0>(rcu));
										//Answer
										sender.push(++unique_key, mo::address, std::get<0>(rcu), 2, "LOGGED IN");
									} else {
										sender.push(++unique_key, mo::address, std::get<0>(rcu), 1, "WRONG PASSWORD");
									}
								} else {
									sender.push(++unique_key, mo::address, std::get<0>(rcu), 1, "NO SUCH USERNAME");
								}
							}
						}
						break;
					case 4: //Logout
						//"addr_minid" is not needed because is did
						if (addr_translation.contains(std::get<0>(rcu))) {
							//Saving addr_translation_erased
							addr_translation_erased[std::get<0>(rcu)] = addr_translation[std::get<0>(rcu)];
							//Logout
							addr_translation.erase(std::get<0>(rcu));
							addr_minid.erase(std::get<0>(rcu));
						}
						break;
					case 5: //Get local online
					{
						/*if (scomm::saddresses.contains(std::get<0>(rcu))) {
							//TODO: replace \r\n by \n
						} else {

						}*/
						std::string bresult;
						unsigned char sz;
						for (auto &pr : addr_translation) {
							sz = pr.second.size();
							bresult.append(reinterpret_cast<char *>(&sz), sizeof(char));
							bresult.append(pr.second);
						}
						sender.push(++unique_key, mo::address, std::get<0>(rcu), 5, bresult);
					}
					break;
					case 6: //Get global online
					{
						/*for (auto &server : scomm::saddresses)
							sender.push(++unique_key, mo::address, server, 5, nullptr);
							//4f4e04a5f8ffded8609f0dd98c39ce3c
						*/
						sender.push(++unique_key, mo::address, std::get<0>(rcu), 1, "NOT IMPLEMENTED");
					}
					break;
					case 7: //Send local message
						if (addr_translation.contains(std::get<0>(rcu))) {
							if (addr_minid[std::get<0>(rcu)] < std::get<1>(rcu)) {
								addr_minid[std::get<0>(rcu)] = std::get<1>(rcu);
								std::string binstr = std::get<3>(rcu);
								//6 - min size of local send cmd: 0-3(date),4(dest-login-len),5+(dest-login),5++(message)
								if (binstr.length() > 6 &&
									binstr.at(4) != 0 &&
									static_cast<unsigned char>(binstr.at(4)) + 5 < binstr.length()) {

									std::string dst_login = binstr.substr(5, static_cast<unsigned char>(binstr.at(4)));
									if (profiles.contains(dst_login)) {
										auto &login = addr_translation[std::get<0>(rcu)];
										std::string binmessage = binstr.substr(0, 4); //date of send
										unsigned char sz = login.size();
										binmessage.append(reinterpret_cast<char *>(&sz), sizeof(char)); //size of sender's login
										binmessage += login; //sender's login
										binmessage += binstr.substr(5 + static_cast<unsigned char>(binstr.at(4)));
										if (addr_translation.contains(profiles[dst_login].address)) { //Target client is online
											sender.push(++unique_key, mo::address, profiles[dst_login].address, 7, binmessage);
										} else { //Target client is offline
											profiles[dst_login].pending_msg(std::get<0>(rcu), 7, binmessage);
										}
										sender.push(++unique_key, mo::address, std::get<0>(rcu), 2, "MESSAGE IS SENT");
									} else {
										sender.push(++unique_key, mo::address, std::get<0>(rcu), 1, "LOCAL LOGIN NOT FOUND");
									}
								}
							}
						} else {
							sender.push(++unique_key, mo::address, std::get<0>(rcu), 1, "NOT LOGGED IN");
						}
						break;
					case 8: //Send local broadcast message
						if (addr_translation.contains(std::get<0>(rcu))) {
							if (addr_minid[std::get<0>(rcu)] < std::get<1>(rcu)) {
								addr_minid[std::get<0>(rcu)] = std::get<1>(rcu);
								std::string binstr = std::get<3>(rcu);
								if (binstr.length() > 4) {
									auto &login = addr_translation[std::get<0>(rcu)];
									std::string binmessage = binstr.substr(0, 4); //date of send
									unsigned char sz = login.size();
									binmessage.append(reinterpret_cast<char *>(&sz), sizeof(char)); //size of sender's login
									binmessage += login; //sender's login
									binmessage += binstr.substr(4);
									for (auto &pr : profiles)
										if (addr_translation.contains(pr.second.address))
											sender.push(++unique_key, mo::address, pr.second.address, 8, binmessage);
										else {
											pr.second.pending_msg(std::get<0>(rcu), 8, binmessage);
										}
									sender.push(++unique_key, mo::address, std::get<0>(rcu), 2, "MESSAGE IS SENT");
								}
							}
						} else {
							sender.push(++unique_key, mo::address, std::get<0>(rcu), 1, "NOT LOGGED IN");
						}
						break;
					case 9: //Send local broadcast to online clients only
						if (addr_translation.contains(std::get<0>(rcu))) {
							if (addr_minid[std::get<0>(rcu)] < std::get<1>(rcu)) {
								addr_minid[std::get<0>(rcu)] = std::get<1>(rcu);
								std::string binstr = std::get<3>(rcu);
								if (binstr.length() > 4) {
									auto &login = addr_translation[std::get<0>(rcu)];
									std::string binmessage = binstr.substr(0, 4); //date of send
									unsigned char sz = login.size();
									binmessage.append(reinterpret_cast<char *>(&sz), sizeof(char)); //size of sender's login
									binmessage += login; //sender's login
									binmessage += binstr.substr(4);
									for (auto &pr : addr_translation)
										sender.push(++unique_key, mo::address, pr.first, 9, binmessage);
									sender.push(++unique_key, mo::address, std::get<0>(rcu), 2, "MESSAGE IS SENT");
								}
							}
						} else {
							sender.push(++unique_key, mo::address, std::get<0>(rcu), 1, "NOT LOGGED IN");
						}
						break;
					default:
						break;
				}
			}

			//Sheduling pending packets to send
			for (auto &profile : profiles) {
				if (addr_translation.contains(profile.second.address)) {
					for (auto &pack : profile.second.get_pendings()) {
						sender.push(++unique_key, std::get<0>(pack), profile.second.address, std::get<1>(pack), std::get<2>(pack));
					}
					profile.second.clear_pendings();
				}
			}

			//Sending sheduled packets
			for (auto& pack : sender.get_snapshot()) {
				switch (std::get<2>(pack)) {
					case 7: //Directed local message
					case 8: //Broadcast local
					case 9: //Broadcast local to onlines
						if (addr_translation.contains(std::get<1>(pack))) {
							std::string &data = std::get<3>(pack);
							mo::moconnector->SendUDP(mo::moconnector->Unicast, data.c_str(), data.length(), std::get<0>(pack), std::get<2>(pack), false, std::get<1>(pack));
						} else if (addr_translation_erased.contains(std::get<1>(pack))) { //Client offline
							//Restoring login-based send form address-based
							std::string &data = std::get<3>(pack);
							profiles[addr_translation_erased[std::get<1>(pack)]].pending_msg(std::get<0>(pack), std::get<2>(pack), data.substr(5));
							sender.erase(*reinterpret_cast<const uint32_t *>(std::get<3>(pack).c_str()));
						} else {
							sender.erase(*reinterpret_cast<const uint32_t *>(std::get<3>(pack).c_str()));
						}
						break;
					default: //Another types of packet
						std::string &data = std::get<3>(pack);
						if (addr_translation.contains(std::get<1>(pack))) {
							mo::moconnector->SendUDP(mo::moconnector->Unicast, data.c_str(), data.length(), std::get<0>(pack), std::get<2>(pack), false, std::get<1>(pack));
						} else { //Removing packet from sending list, client lost
							if (addr_packanswers_graylist.contains(std::get<1>(pack))) {
								if (--addr_packanswers_graylist[std::get<1>(pack)]) {
									mo::moconnector->SendUDP(mo::moconnector->Unicast, data.c_str(), data.length(), std::get<0>(pack), std::get<2>(pack), false, std::get<1>(pack));
								} else {
									sender.erase(*reinterpret_cast<const uint32_t *>(std::get<3>(pack).c_str()));
									addr_packanswers_graylist.erase(std::get<1>(pack));
									mo::moconnector->SendUDP(mo::moconnector->Unicast, data.c_str(), data.length(), std::get<0>(pack), std::get<2>(pack), false, std::get<1>(pack));
								}
							} else {
								addr_packanswers_graylist[std::get<1>(pack)] = 2;
								mo::moconnector->SendUDP(mo::moconnector->Unicast, data.c_str(), data.length(), std::get<0>(pack), std::get<2>(pack), false, std::get<1>(pack));
							}
						}
						break;
				}
			}

			clibs::addr_translation_mutex.unlock();
		}
	}
}