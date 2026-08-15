#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <thread>
#include <atomic>

#pragma comment(lib, "ws2_32.lib")

namespace AnshuBio {

class UDPBeacon {
public:
    UDPBeacon();
    ~UDPBeacon();

    bool Start(int port = 42424);
    void Stop();

private:
    void BroadcastLoop();
    void ListenLoop();

    SOCKET m_broadcastSocket = INVALID_SOCKET;
    SOCKET m_listenSocket = INVALID_SOCKET;
    int m_port = 42424;
    std::atomic<bool> m_isRunning{false};
    std::thread m_broadcastThread;
    std::thread m_listenThread;
};

} // namespace AnshuBio
