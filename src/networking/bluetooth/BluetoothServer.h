#pragma once
#include <winsock2.h>
#include <ws2bth.h>
#include <string>
#include <thread>
#include <atomic>
#include "BluetoothHelper.h"

namespace AnshuBio {

class AuthCoordinator;

class BluetoothServer {
public:
    explicit BluetoothServer(AuthCoordinator* coordinator);
    ~BluetoothServer();

    bool Start();
    void Stop();

    bool IsRadioAvailable() const;
    bool IsConnected() const;

private:
    void ServerLoop();
    void HandleClient(SOCKET clientSocket);

    AuthCoordinator* m_coordinator;
    SOCKET m_serverSocket = INVALID_SOCKET;
    std::atomic<bool> m_isRunning{false};
    std::atomic<bool> m_hasActiveConnection{false};
    bool m_isRadioAvailable = false;
    std::thread m_serverThread;
};

} // namespace AnshuBio
