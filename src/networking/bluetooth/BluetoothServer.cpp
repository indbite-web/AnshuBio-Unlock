#include "BluetoothServer.h"
#include "../../core/Constants.h"
#include "../../core/AuthCoordinator.h"
#include "../../storage/SecurityLogger.h"

// Define custom AnshuBio RFCOMM GUID: {1b7e8251-2877-41c3-b46e-cf057c562023}
static const GUID s_AnshuBioRfcommGuid = { 0x1b7e8251, 0x2877, 0x41c3, { 0xb4, 0x6e, 0xcf, 0x05, 0x7c, 0x56, 0x20, 0x23 } };

namespace AnshuBio {

BluetoothServer::BluetoothServer(AuthCoordinator* coordinator) : m_coordinator(coordinator) {}

BluetoothServer::~BluetoothServer() {
    Stop();
}

bool BluetoothServer::Start() {
    if (m_isRunning) return true;

    m_isRadioAvailable = BluetoothHelper::IsBluetoothRadioAvailable();
    if (!m_isRadioAvailable) {
        SecurityLogger::Instance().Info("BLE", "Bluetooth radio unavailable or turned off. Wi-Fi LAN is operating as primary transport.");
        return false;
    }

    m_isRunning = true;
    m_serverThread = std::thread(&BluetoothServer::ServerLoop, this);
    SecurityLogger::Instance().Info("BLE", "Native Windows Bluetooth RFCOMM server initialized");
    return true;
}

void BluetoothServer::Stop() {
    if (!m_isRunning) return;
    m_isRunning = false;
    m_hasActiveConnection = false;

    if (m_serverSocket != INVALID_SOCKET) {
        closesocket(m_serverSocket);
        m_serverSocket = INVALID_SOCKET;
    }

    if (m_serverThread.joinable()) {
        m_serverThread.join();
    }
    SecurityLogger::Instance().Info("BLE", "Bluetooth / BLE Service stopped");
}

bool BluetoothServer::IsRadioAvailable() const {
    return m_isRadioAvailable;
}

bool BluetoothServer::IsConnected() const {
    return m_hasActiveConnection;
}

void BluetoothServer::ServerLoop() {
    m_serverSocket = socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM);
    if (m_serverSocket == INVALID_SOCKET) {
        m_isRunning = false;
        return;
    }

    SOCKADDR_BTH address{};
    address.addressFamily = AF_BTH;
    address.port = BT_PORT_ANY;

    if (bind(m_serverSocket, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR) {
        closesocket(m_serverSocket);
        m_serverSocket = INVALID_SOCKET;
        m_isRunning = false;
        return;
    }

    // Register SDP Service
    BluetoothHelper::RegisterSDPService(m_serverSocket, "AnshuBio Unlock", s_AnshuBioRfcommGuid);

    if (listen(m_serverSocket, 3) == SOCKET_ERROR) {
        closesocket(m_serverSocket);
        m_serverSocket = INVALID_SOCKET;
        m_isRunning = false;
        return;
    }

    while (m_isRunning) {
        SOCKADDR_BTH clientAddr{};
        int clientLen = sizeof(clientAddr);
        SOCKET clientSocket = accept(m_serverSocket, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);

        if (clientSocket != INVALID_SOCKET && m_isRunning) {
            m_hasActiveConnection = true;
            std::thread clientThread(&BluetoothServer::HandleClient, this, clientSocket);
            clientThread.detach();
        }
    }
}

void BluetoothServer::HandleClient(SOCKET clientSocket) {
    char buffer[4096] = { 0 };
    int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

    if (bytesRead > 0) {
        buffer[bytesRead] = '\0';
        std::string rawJson(buffer);

        if (m_coordinator) {
            std::string response = m_coordinator->ProcessIncomingJsonMessage(rawJson, "Bluetooth/RFCOMM");
            response += "\n";
            send(clientSocket, response.c_str(), static_cast<int>(response.length()), 0);
        }
    }

    closesocket(clientSocket);
    m_hasActiveConnection = false;
}

} // namespace AnshuBio
