#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include "UDPBeacon.h"

namespace AnshuBio {

class AuthCoordinator;

class WiFiServer {
public:
    explicit WiFiServer(AuthCoordinator* coordinator);
    ~WiFiServer();

    bool Start(int port = 42425);
    void Stop();

private:
    void ServerLoop();
    void HandleClient(SOCKET clientSocket);
    std::string HandleHttpRequest(const std::string& method, const std::string& path, const std::string& body, const std::string& remoteIp);

    AuthCoordinator* m_coordinator;
    SOCKET m_serverSocket = INVALID_SOCKET;
    int m_port = 42425;
    std::atomic<bool> m_isRunning{false};
    std::thread m_serverThread;
    UDPBeacon m_udpBeacon;
};

} // namespace AnshuBio
