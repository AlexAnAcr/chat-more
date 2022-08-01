#include <iostream>
#include <string>
#include <time.h>

#include "mo.h"
#include "subcom.h"

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

int main()
{
    srand(time(NULL));

    cout << "==== WELCOME TO THE DEBUGGER ====" << endl;

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
            cout << "Enter the Port of " << input <<" >";
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

            subcom::enable = true;
            
            thread_clwor = thread(subcom::ProLoop);
			thread_recv = thread(mo::ReceiveLoop);
			thread_resend = thread(mo::SendLoop);
			thread_thtcp = thread(subcom::StartProc);

            state = 2;
            goto reload_menu;
        }
        case 2:
            cout << endl << "Select a option:" << endl;
            cout << "1) disconnect" << endl;
            cout << "2) get my address" << endl;
            cout << "3) make a wish-string" << endl;
            cout << "4) select a message server" << endl;
            cout << "5) get onlines at all" << endl;
            cout << "6) exit" << endl;
            std::getline(std::cin, input);
            switch (StrToOptionNumber(input)) {
                case 1:
                    subcom::enable = false;
                    thread_clwor.~thread();
                    thread_recv.~thread();
                    thread_resend.~thread();
                    thread_thtcp.~thread();
                    mo::moconnector->Finalise();

                    cout << "Disconnected!" << endl;
                    state = 0;
                    goto reload_menu;
                case 2:
                    cout << "Current address: " << static_cast<unsigned short>(mo::address) << endl;
                    goto reload_menu;
                case 3:
					cout << "Wish-string contains of following words: " << endl;
					cout << "0) SERVER" << endl
						<< "1) MESSAGE" << endl
						<< "2) PRIVATE" << endl
						<< "3) RELIABLE" << endl
						<< "4) QUALITATIVE" << endl
						<< "5) FAST" << endl
						<< "6) VERY" << endl
						<< "7) PRO" << endl
						<< "8) EFFECTIVE" << endl
						<< "9) ORIGINAL";
                    cout << "Make string by typing indexes (like 01132): ";
                    std::getline(std::cin, input);
                    cout << "Gotten string: ";
                    {
                        subcom::message = "";
                        for (auto &ch : input) {
                            subcom::message += subcom::capables[StrToOptionNumber(ch)];
                            subcom::message += ' ';
                        }
                        cout << subcom::message << endl;
                    }
                    goto reload_menu;
                case 4:
                {
                    cout << "Current list of avail servers:" << endl;
                    for (auto &srv : subcom::servers) {
                        cout << '[' << static_cast<unsigned char>(srv.first) << "] MSG: " << srv.second << endl;
                    }
                    cout << "Enter the address of server: " << endl;
                    std::getline(std::cin, input);
                    subcom::server_address = StrToOptionNumber(input);
                    state = 3;
                    goto reload_menu;
                }
                case 5:
                {
                    cout << "Current list of online users(and servers) at all:" << endl;
                    for (auto &user : subcom::whoisonline) {
                        cout << '[' << static_cast<unsigned char>(user) << "] who: " << (subcom::servers.contains(user) ? "SERVER" : "USER") << endl;
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
            cout << endl << "Selected server: " << static_cast<unsigned short>(mo::address) << endl;
            cout << "Sessionless possibilites:" << endl;
            cout << "1) request onlines from the server" << endl;
            cout << "2) request onlines at all" << endl;
            cout << "3) make session" << endl;
            cout << "4) mimic session" << endl;
            cout << "5) update visible address" << endl;
            cout << "6) back to server selection" << endl;
            std::getline(std::cin, input);
            switch (StrToOptionNumber(input)) {
                case 1:
                    mo::sender.push(++minid, mo::address, subcom::server_address, 5, "");
                    cout << "Making request..." << endl;
                    goto reload_menu;
                case 2:
                {
                    cout << "Current list of online users(and servers) at all:" << endl;
                    for (auto &user : subcom::whoisonline) {
                        cout << '[' << static_cast<unsigned char>(user) << "] who: " << (subcom::servers.contains(user) ? "SERVER" : "USER") << endl;
                    }
                    goto reload_menu;
                }
                case 3:
                    
                    goto reload_menu;
                case 4:

                    goto reload_menu;
                case 5:
                    cout << "OK!" << endl;
                    subcom::visible_server_address = static_cast<char>(subcom::server_address);
                    goto reload_menu;
                case 6:
                {
                    state = 2;
                    goto reload_menu;
                }
                goto reload_menu;
                case 0:
                    goto reload_menu;
                default:
                    return 0;
            }
            break;
        default:
            break;
    }

    return 0;
}
