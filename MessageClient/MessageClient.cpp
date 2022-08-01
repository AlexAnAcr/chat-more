#include <iostream>
#include <string>
#include <time.h>
#include "mconn.h"
#include "scomm.h"
#include "msg_proc.h"

using namespace std;

extern void comain();

static inline unsigned char StrToOptionNumber(const string &num) {
	if (num.length() == 0 || num[0] < 48 || num[0] > 57) return 0;
	return num[0] - 48;
}
static inline unsigned char StrToOptionNumber(char num) {
	if (num < 48 || num > 57) return 0;
	return num - 48;
}

static uint32_t YMDHMStoStruct(const std::tm &datetime) {
	uint32_t bytes = 0;

	bytes |= (datetime.tm_year - 122) << 26;
	bytes |= (datetime.tm_mon) << 22;
	bytes |= (datetime.tm_mday - 1) << 17;
	bytes |= (datetime.tm_hour * 3600 + datetime.tm_min * 60 + datetime.tm_sec);

	return bytes;
}

int main() {
	std::ios::sync_with_stdio(false);

	srand(time(NULL));

	cout << "==== WELCOME TO MESSAGE CLIENT ====" << endl;

	unsigned char state = 0;
	std::string input;

	uint32_t minid = 0;


	thread thread_clwor;
	thread thread_recv;
	thread thread_resend;
	thread thread_thtcp;

reload_menu:;
	switch (state) {
		case 0:
			cout << endl << "Select a option:" << endl;
			cout << "1) connect" << endl;
			cout << "2) exit" << endl;
			std::getline(std::cin, input);
			switch (StrToOptionNumber(input)) {
				case 1:
					state = 1;
					goto reload_menu;
				case 0:
					goto reload_menu;
				default:
					return 0;
					break;
			}
			break;
		case 1:
		{
			cout << "Enter IP address of a server >";
			std::getline(std::cin, input);

			mo::ip = input;
			cout << "Enter the Port of " << input << " >";
			std::getline(std::cin, input);

			try {
				mo::port = stoi(input);
			} catch (const std::exception &ex) {
				cout << ex.what() << endl;
				state = 0;
				goto reload_menu;
			}

			try {
				comain();
				cout << "Connected!" << endl;
			} catch (const std::exception &ex) {
				cout << ex.what() << endl;
				state = 0;
				goto reload_menu;
			}

			scomm::selected_server = -1;
			scomm::enable = true;

			thread_clwor = std::move(thread(scomm::StatProcLoop));
			thread_recv = std::move(thread(mo::ReceiveLoop));

			cout << "Waiting for message server..." << endl;

			do {
				cout << "Do you want to interrupt? (Y/n) " << endl;
				std::getline(std::cin, input);
				if (scomm::selected_server == -1) {
					if (input.length() != 0 && (input[0] == 'Y' || input[0] == 'y')) {
						scomm::enable = false;
						thread_clwor.~thread();
						thread_recv.~thread();
						state = 0;
						goto reload_menu;
					}
				}
			} while (scomm::selected_server == -1);

			thread_resend = std::move(thread(mo::SendLoop));
			thread_thtcp = std::move(thread(scomm::StartTCProc));

			state = 2;
			goto reload_menu;
		}
		case 2:
			cout << endl << "Select a option:" << endl;
			cout << "1) register" << endl;
			cout << "2) login" << endl;
			std::getline(std::cin, input);
			switch (StrToOptionNumber(input)) {
				case 1:
				{
					std::string login, password;
					cout << "Enter new login: ";
					std::getline(std::cin, login);

					if (login == "") {
						cout << "Login can't be empty!" << endl;
						goto reload_menu;
					}

					cout << "Enter new password: ";
					std::getline(std::cin, password);

					if (password == "") {
						cout << "Password can't be empty!" << endl;
						goto reload_menu;
					}

					cout << "Re-enter new password: ";
					std::getline(std::cin, input);

					if (input != password) {
						cout << "Passwords are not same!" << endl;
						goto reload_menu;
					}

					unsigned char temp = login.length();
					string binstr(reinterpret_cast<char *>(&temp), sizeof(char));
					binstr += login;
					binstr += password;
					mo::sender.push(++minid, mo::address, scomm::selected_server, 1, binstr);
					cout << "Making request..." << endl;

					auto result = msg::RetriveResultByType();
					if (get<0>(result) == 2) {
						cout << "Successfully registered!" << endl;
						state = 3;
					} else {
						cout << "Error: " << get<1>(result) << endl;
					}
					goto reload_menu;
				}
				case 2:
				{
					std::string login, password;
					cout << "Enter the login: ";
					std::getline(std::cin, login);

					if (login == "") {
						cout << "Login can't be empty!" << endl;
						goto reload_menu;
					}

					cout << "Enter the password: ";
					std::getline(std::cin, password);

					if (password == "") {
						cout << "Password can't be empty!" << endl;
						goto reload_menu;
					}

					unsigned char temp = login.length();
					string sendstream(reinterpret_cast<char *>(&temp), sizeof(char));
					sendstream += login;
					sendstream += password;
					mo::sender.push(++minid, mo::address, scomm::selected_server, 3, sendstream);
					cout << "Making request..." << endl;

					auto result = msg::RetriveResultByType();
					if (get<0>(result) == 2) {
						cout << "Successfully logged in!" << endl;
						state = 3;
					} else {
						cout << "Error: " << get<1>(result) << endl;
					}
					goto reload_menu;
				}
				case 0:
					goto reload_menu;
				default:
					return 0;
					break;
			}
			break;
		case 3:
			cout << endl << "Select a option:" << endl;
			cout << "1) unregister" << endl;
			cout << "2) log off" << endl;
			cout << "3) write a message" << endl;
			cout << "4) read a messages" << endl;
			std::getline(std::cin, input);
			switch (StrToOptionNumber(input)) {
				case 1:
				{
					cout << "Enter the password: ";
					std::getline(std::cin, input);

					mo::sender.push(++minid, mo::address, scomm::selected_server, 2, input);
					cout << "Making request..." << endl;

					auto result = msg::RetriveResultByType();
					if (get<0>(result) == 2) {
						cout << "Successfully unregistered!" << endl;
						state = 2;
					} else {
						cout << "Error: " << get<1>(result) << endl;
					}
					goto reload_menu;
				}
				case 2:
				{
					mo::sender.push(++minid, mo::address, scomm::selected_server, 4, "");
					cout << "Logged out." << endl;
					state = 2;
					goto reload_menu;
				}
				case 3:
				{
					vector<string> onlines;
					{
						mo::sender.push(++minid, mo::address, scomm::selected_server, 5, "");
						cout << "Getting onlines..." << endl;
						auto result = get<1>(msg::RetriveResultByType(5));
						unsigned short i = 0;
						while (result.length() - i > 0) {
							onlines.emplace_back(result.substr(i + 1, result[i]));
							i += result[i] + 1;
						}
						cout << "Currently online:" << endl;
						for (auto &item : onlines) {
							cout << '[' << item << ']' << endl;
						}
					}
					cout << endl << "You can enter any username - not just from the list. If the username you entered turns out to be incorrect, an error message will be displayed. You also can use special usernames @all and @online to send to a users." << endl;
					cout << "Enter the destignated username: ";
					string login;
					std::getline(std::cin, login);

					cout << "Enter the message: ";
					std::getline(std::cin, input);

					time_t timet = time(nullptr);
					tm currtime;
					localtime_s(&currtime, &timet);
					uint32_t stm = YMDHMStoStruct(currtime);
					string binstr(reinterpret_cast<char *>(&stm), sizeof(uint32_t));
					if (login == "@all") {
						binstr += input;
						mo::sender.push(++minid, mo::address, scomm::selected_server, 8, binstr);
						if (!msg::archive.contains("@all"))
							msg::archive.insert(std::make_pair("@all", msg::interlocutor()));
						msg::archive["@all"].outcoming.emplace_back(input, currtime, 1);
						cout << "Sending broadcast message to all..." << endl;
					} else if (login == "@online") {
						binstr += input;
						mo::sender.push(++minid, mo::address, scomm::selected_server, 9, binstr);
						for (auto& item : onlines) {
							if (!msg::archive.contains(item))
								msg::archive.insert(std::make_pair(item, msg::interlocutor()));
							msg::archive[item].outcoming.emplace_back(input, currtime, 2);
						}
						cout << "Sending broadcast message to onlines..." << endl;
					} else {
						if (login == "") {
							cout << "Login can't be empty!" << endl;
							goto reload_menu;
						}

						unsigned char temp = login.length();
						binstr.append(reinterpret_cast<char *>(&temp), sizeof(char));
						binstr += login;
						binstr += input;
						mo::sender.push(++minid, mo::address, scomm::selected_server, 7, binstr);
						if (!msg::archive.contains(login))
							msg::archive.insert(std::make_pair(login, msg::interlocutor()));
						msg::archive[login].outcoming.emplace_back(input, currtime);
						cout << "Sending message to \"" << login << "\"..." << endl;
					}

					auto result = msg::RetriveResultByType();
					if (get<0>(result) == 2) {
						cout << "Mesage was successfully sent!" << endl;
					} else {
						cout << "Error: " << get<1>(result) << endl;
					}
					goto reload_menu;
				}
				case 4:
					state = 4;
					goto reload_menu;
				case 0:
					goto reload_menu;
				default:
					return 0;
					break;
			}
			break;
		case 4:
			cout << endl << "Select a option:" << endl;
			cout << "1) go to previous menu" << endl;
			cout << "2) list incoming messages" << endl;
			cout << "3) list outcoming messages" << endl;
			std::getline(std::cin, input);
			switch (StrToOptionNumber(input)) {
				case 1:
					state = 3;
					goto reload_menu;
				case 2:
				{
					cout << "List of known logins:" << endl;
					for (auto &item : msg::archive) {
						cout << '[' << item.first << "] incomings count: " << item.second.incoming.size() << (!item.second.incoming.size() ? "" : ((item.second.incoming.cend() - 1)->readen ? "" : " [NEW]")) << endl;
					}
					cout << endl << "If the username you entered turns out to be incorrect, an error message will be displayed." << endl;
					cout << "Enter the destignated username: ";
					std::getline(std::cin, input);

					if (!msg::archive.contains(input)) {
						cout << "Entered login does not exists!" << endl;
						goto reload_menu;
					}

					cout << "List of received messages:" << endl;
					vector<msg::message> &ilc = msg::archive[input].incoming;
					for (unsigned int i = 0, sz = ilc.size(); i != sz; ++i) {
						msg::message &item = ilc[i];
						cout << (item.readen ? "" : "[NEW] ") << (1900 + item.datetime.tm_year) << '-' << (item.datetime.tm_mon + 1) << '-' << item.datetime.tm_mday << ' ' << item.datetime.tm_hour << ':' << item.datetime.tm_min << ':' << item.datetime.tm_sec << "\t"
							<< item.msg << endl;
						item.readen = true;
					}
					goto reload_menu;
				}
				case 3:
				{
					cout << "List of known logins:" << endl;
					for (auto &item : msg::archive) {
						cout << '[' << item.first << "] outcomings count: " << item.second.outcoming.size() << endl;
					}
					cout << endl << "If the username you entered turns out to be incorrect, an error message will be displayed. You also can use special username @all." << endl;
					cout << "Enter the destignated username: ";
					std::getline(std::cin, input);

					if (!msg::archive.contains(input)) {
						cout << "Entered login does not exists!" << endl;
						goto reload_menu;
					}

					cout << "List of sent messages:" << endl;
					for (const auto& item : msg::archive[input].outcoming) {
						cout << (1900 + item.datetime.tm_year) << '-' << (item.datetime.tm_mon + 1) << '-' << item.datetime.tm_mday << ' ' << item.datetime.tm_hour << ':' << item.datetime.tm_min << ':' << item.datetime.tm_sec << "\t"
							<< item.msg << endl;
					}
					goto reload_menu;
				}
				case 0:
					goto reload_menu;
				default:
					return 0;
					break;
			}
			break;
		default:
			break;
	}

	return 0;
}
