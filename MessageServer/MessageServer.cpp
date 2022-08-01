#include <iostream>
#include <thread>
#include <string>

#include "mconn.h"
#include "servers_communication.h"
#include "clients_basis.h"

extern void comain();

using namespace std;

int main()
{
    cout << "==== Message Processing Server ====" << endl;
    cout << "Enter Commutation Server IP: ";
    string ipaddr;
    cin >> ipaddr;
    mo::ip = ipaddr.c_str();
    cout << "Enter Commutation Server port: ";
    cin >> mo::port;
    try {
        comain();
    } catch (const std::exception &ex) {
        cout << ex.what() << endl;
        return -1;
    }

    std::thread announcer(scomm::ServerIdBroadcasteLoop);
    SetThreadPriority(announcer.native_handle(), THREAD_PRIORITY_BELOW_NORMAL);
    
    std::thread udp_receiver(mo::ReceiveLoop);
    SetThreadPriority(udp_receiver.native_handle(), THREAD_PRIORITY_ABOVE_NORMAL);

    std::thread req_processor(clibs::ClientProcessingLoop);
    SetThreadPriority(req_processor.native_handle(), THREAD_PRIORITY_NORMAL);

    std::thread udp_resender([](){
        while (true) {
            Sleep(250);
            clibs::wait.notify_one();
        }
    });
    SetThreadPriority(udp_resender.native_handle(), THREAD_PRIORITY_BELOW_NORMAL);

    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);

    auto in_dtype = mo::moconnector->Broadcast;
    unsigned short bufflen = 16, bufflen_max = 0;
    char unicast_addr, u4bits, *buffer = new char[bufflen];
    while (mo::moconnector->ReceiveTCP(in_dtype, unicast_addr, u4bits, buffer, bufflen)) {
        if (bufflen > bufflen_max) bufflen_max = bufflen;
        if (in_dtype == mo::moconnector->ReceiveSpecilalTCP_Indexes) {
            std::unordered_set<char> onlines;
            for (char* ch = buffer, *ec = buffer + bufflen; ch != ec; ++ch) {
                onlines.insert(*ch);
            }
            auto vec = std::vector<std::pair<char, std::string>>(clibs::addr_translation.cbegin(), clibs::addr_translation.cend());
            clibs::addr_translation_mutex.lock();
            for (auto &val : vec) {
                if (!onlines.contains(val.first)) {
                    clibs::addr_translation.erase(val.first);
                }
            }
            clibs::addr_translation_mutex.unlock();
        }
    }
    delete[] buffer;

    std::cout << "Control protocol error occured: finishing\n";

    return 0;
}