#include "UDPBeacon.h"
#include "../../core/Constants.h"
#include "../../storage/SecurityLogger.h"
#include "../../storage/KeyStore.h"
#include "../../session/SessionMonitor.h"
#include <sstream>

namespace AnshuBio {

UDPBeacon::UDPBeacon() {}

UDPBeacon::~UDPBeacon() {
    Stop();
}

bool UDPBeacon::Start(int port) {
    if (m_isRunning) return true;
    m_port = port;
    m_isRunning = true;

    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    m_broadcastThread = std::thread(&UDPBeacon::BroadcastLoop, this);
    m_listenThread = std::thread(&UDPBeacon::ListenLoop, this);

    SecurityLogger::Instance().Info("WIFI", "Native UDP Discovery Beacon active on port " + std::to_string(m_port));
    return true;
}

void UDPBeacon::Stop() {
    if (!m_isRunning) return;
    m_isRunning = false;

    if (m_broadcastSocket != INVALID_SOCKET) {
        closesocket(m_broadcastSocket);
        m_broadcastSocket = INVALID_SOCKET;
    }
    if (m_listenSocket != INVALID_SOCKET) {
        closesocket(m_listenSocket);
        m_listenSocket = INVALID_SOCKET;
    }

    if (m_broadcastThread.joinable()) m_broadcastThread.join();
    if (m_listenThread.joinable()) m_listenThread.join();

    WSACleanup();
}

void UDPBeacon::BroadcastLoop() {
    m_broadcastSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_broadcastSocket == INVALID_SOCKET) return;

    BOOL broadcastEnable = TRUE;
    setsockopt(m_broadcastSocket, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<const char*>(&broadcastEnable), sizeof(broadcastEnable));

    sockaddr_in broadcastAddr{};
    broadcastAddr.sin_family = AF_INET;
    broadcastAddr.sin_port = htons(static_cast<u_short>(m_port));
    broadcastAddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    while (m_isRunning) {
        auto identity = KeyStore::Instance().GetPcIdentity();
        auto displayName = KeyStore::Instance().GetPcDisplayName();
        auto stateStr = SessionStateToString(SessionMonitor::Instance().GetCurrentState());

        std::stringstream ss;
        ss << "{\"type\":\"DISCOVERY_BEACON\",\"pcId\":\"" << identity.pcId
           << "\",\"pcName\":\"" << displayName << "\",\"serverPort\":" << Network::SERVER_PORT
           << ",\"version\":\"1.0.0\",\"status\":\"" << stateStr << "\"}";

        std::string payload = ss.str();
        sendto(m_broadcastSocket, payload.c_str(), static_cast<int>(payload.length()), 0,
               reinterpret_cast<sockaddr*>(&broadcastAddr), sizeof(broadcastAddr));

        std::this_thread::sleep_for(std::chrono::milliseconds(Network::BROADCAST_INTERVAL_MS));
    }
}

void UDPBeacon::ListenLoop() {
    m_listenSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_listenSocket == INVALID_SOCKET) return;

    BOOL reuse = TRUE;
    setsockopt(m_listenSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), sizeof(reuse));

    sockaddr_in listenAddr{};
    listenAddr.sin_family = AF_INET;
    listenAddr.sin_port = htons(static_cast<u_short>(m_port));
    listenAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(m_listenSocket, reinterpret_cast<sockaddr*>(&listenAddr), sizeof(listenAddr)) == SOCKET_ERROR) {
        return;
    }

    char buffer[2048];
    sockaddr_in clientAddr{};
    int clientLen = sizeof(clientAddr);

    while (m_isRunning) {
        int bytes = recvfrom(m_listenSocket, buffer, sizeof(buffer) - 1, 0,
                             reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
        if (bytes > 0) {
            buffer[bytes] = '\0';
            std::string msg(buffer);

            if (msg.find("DISCOVERY_QUERY") != std::string::npos) {
                auto identity = KeyStore::Instance().GetPcIdentity();
                auto displayName = KeyStore::Instance().GetPcDisplayName();

                std::stringstream resp;
                resp << "{\"type\":\"DISCOVERY_RESPONSE\",\"pcId\":\"" << identity.pcId
                     << "\",\"pcName\":\"" << displayName << "\",\"serverPort\":" << Network::SERVER_PORT
                     << ",\"version\":\"1.0.0\"}";

                std::string respStr = resp.str();
                sendto(m_listenSocket, respStr.c_str(), static_cast<int>(respStr.length()), 0,
                       reinterpret_cast<sockaddr*>(&clientAddr), clientLen);
            }
        }
    }
}

} // namespace AnshuBio
