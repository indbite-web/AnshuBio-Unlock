#include "WiFiServer.h"
#include "../../core/Constants.h"
#include "../../core/AuthCoordinator.h"
#include "../../storage/SecurityLogger.h"
#include "../../storage/KeyStore.h"
#include "../../session/SessionMonitor.h"
#include <sstream>

namespace AnshuBio {

WiFiServer::WiFiServer(AuthCoordinator* coordinator) : m_coordinator(coordinator) {}

WiFiServer::~WiFiServer() {
    Stop();
}

bool WiFiServer::Start(int port) {
    if (m_isRunning) return true;
    m_port = port;
    m_isRunning = true;

    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    m_udpBeacon.Start(Network::DISCOVERY_PORT);

    m_serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_serverSocket == INVALID_SOCKET) {
        m_isRunning = false;
        return false;
    }

    BOOL reuse = TRUE;
    setsockopt(m_serverSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), sizeof(reuse));

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(static_cast<u_short>(m_port));
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(m_serverSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        closesocket(m_serverSocket);
        m_serverSocket = INVALID_SOCKET;
        m_isRunning = false;
        return false;
    }

    if (listen(m_serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        closesocket(m_serverSocket);
        m_serverSocket = INVALID_SOCKET;
        m_isRunning = false;
        return false;
    }

    m_serverThread = std::thread(&WiFiServer::ServerLoop, this);
    SecurityLogger::Instance().Info("WIFI", "Native offline HTTP/Protocol server active on port " + std::to_string(m_port));
    return true;
}

void WiFiServer::Stop() {
    if (!m_isRunning) return;
    m_isRunning = false;

    m_udpBeacon.Stop();

    if (m_serverSocket != INVALID_SOCKET) {
        closesocket(m_serverSocket);
        m_serverSocket = INVALID_SOCKET;
    }

    if (m_serverThread.joinable()) {
        m_serverThread.join();
    }

    WSACleanup();
    SecurityLogger::Instance().Info("WIFI", "Wi-Fi / LAN service stopped");
}

void WiFiServer::ServerLoop() {
    while (m_isRunning) {
        sockaddr_in clientAddr{};
        int clientLen = sizeof(clientAddr);
        SOCKET clientSocket = accept(m_serverSocket, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);

        if (clientSocket != INVALID_SOCKET && m_isRunning) {
            std::thread clientThread(&WiFiServer::HandleClient, this, clientSocket);
            clientThread.detach();
        }
    }
}

void WiFiServer::HandleClient(SOCKET clientSocket) {
    // Set socket receive timeout (5 seconds)
    DWORD timeout = 5000;
    setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));

    char buffer[8192] = { 0 };
    int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

    if (bytesRead > 0) {
        buffer[bytesRead] = '\0';
        std::string rawRequest(buffer);

        std::istringstream reqStream(rawRequest);
        std::string method, path, httpVer;
        reqStream >> method >> path >> httpVer;

        // Parse body if present
        std::string body;
        size_t bodyPos = rawRequest.find("\r\n\r\n");
        if (bodyPos != std::string::npos) {
            body = rawRequest.substr(bodyPos + 4);
        }

        std::string responseBody = HandleHttpRequest(method, path, body, "127.0.0.1");

        std::stringstream respStream;
        respStream << "HTTP/1.1 200 OK\r\n"
                   << "Content-Type: application/json\r\n"
                   << "Content-Length: " << responseBody.length() << "\r\n"
                   << "Connection: close\r\n\r\n"
                   << responseBody;

        std::string fullResp = respStream.str();
        send(clientSocket, fullResp.c_str(), static_cast<int>(fullResp.length()), 0);
    }

    closesocket(clientSocket);
}

std::string WiFiServer::HandleHttpRequest(const std::string& method, const std::string& path, const std::string& body, const std::string& remoteIp) {
    // 1. Info endpoint
    if (path == "/api/info") {
        auto identity = KeyStore::Instance().GetPcIdentity();
        auto displayName = KeyStore::Instance().GetPcDisplayName();
        auto phones = KeyStore::Instance().GetTrustedPhones();
        auto stateStr = SessionStateToString(SessionMonitor::Instance().GetCurrentState());

        std::stringstream ss;
        ss << "{\"product\":\"AnshuBio Unlock\",\"version\":\"1.0.0\",\"pcId\":\"" << identity.pcId
           << "\",\"pcName\":\"" << displayName << "\",\"sessionState\":\"" << stateStr
           << "\",\"trustedPhonesCount\":" << phones.size() << "}";
        return ss.str();
    }

    // 2. Pending challenge poll
    if (path.find("/api/auth/pending") != std::string::npos) {
        if (m_coordinator) {
            return m_coordinator->GetPendingChallengeJson();
        }
        return "{\"pending\":false}";
    }

    // 3. Message endpoint
    if (method == "POST" && m_coordinator) {
        return m_coordinator->ProcessIncomingJsonMessage(body, remoteIp);
    }

    return "{\"status\":\"OK\"}";
}

} // namespace AnshuBio
