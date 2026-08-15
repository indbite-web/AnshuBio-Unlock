#include "NamedPipeServer.h"
#include "../core/Constants.h"
#include "../core/Models.h"
#include "../storage/SecurityLogger.h"
#include "../storage/KeyStore.h"
#include "../session/SessionMonitor.h"
#include <sddl.h>
#include <sstream>

#pragma comment(lib, "advapi32.lib")

namespace AnshuBio {

NamedPipeServer& NamedPipeServer::Instance() {
    static NamedPipeServer s_instance;
    return s_instance;
}

NamedPipeServer::NamedPipeServer() : m_pipeName(Network::PIPE_NAME) {}

NamedPipeServer::~NamedPipeServer() {
    Stop();
}

bool NamedPipeServer::Start() {
    if (m_isRunning) return true;
    m_isRunning = true;
    m_serverThread = std::thread(&NamedPipeServer::ServerLoop, this);
    SecurityLogger::Instance().Info("PIPE", "Native Win32 Named Pipe IPC Server starting on \\\\.\\pipe\\AnshuBioUnlockAuthPipe");
    return true;
}

void NamedPipeServer::Stop() {
    if (!m_isRunning) return;
    m_isRunning = false;
    {
        std::lock_guard<std::mutex> lock(m_authMutex);
        m_authHasResult = true;
        m_authCv.notify_all();
    }

    // Connect dummy client to unblock ConnectNamedPipe
    HANDLE hDummy = CreateFileW(m_pipeName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (hDummy != INVALID_HANDLE_VALUE) {
        CloseHandle(hDummy);
    }

    if (m_serverThread.joinable()) {
        m_serverThread.join();
    }
    SecurityLogger::Instance().Info("PIPE", "Named Pipe Server stopped");
}

void NamedPipeServer::NotifyAuthResolved(bool success, const std::string& phoneId, const std::string& phoneName,
                                        const std::wstring& username, const std::wstring& domain, const std::wstring& password) {
    std::lock_guard<std::mutex> lock(m_authMutex);
    m_authSuccess = success;
    m_authPhoneId = phoneId;
    m_authPhoneName = phoneName;
    m_authUsername = username;
    m_authDomain = domain;
    m_authPassword = password;
    m_authHasResult = true;
    m_authCv.notify_all();
}

void NamedPipeServer::ServerLoop() {
    // Strict ACL: ONLY Local System (SY) and Built-in Administrators (BA)
    SECURITY_ATTRIBUTES sa;
    ZeroMemory(&sa, sizeof(sa));
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = FALSE;

    PSECURITY_DESCRIPTOR pSD = nullptr;
    if (ConvertStringSecurityDescriptorToSecurityDescriptorW(
            L"D:(A;;GA;;;SY)(A;;GA;;;BA)",
            SDDL_REVISION_1,
            &pSD,
            nullptr)) {
        sa.lpSecurityDescriptor = pSD;
    }

    while (m_isRunning) {
        HANDLE hPipe = CreateNamedPipeW(
            m_pipeName.c_str(),
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES,
            sizeof(PipeAuthPacket) + 1024,
            sizeof(PipeAuthPacket) + 1024,
            0,
            sa.lpSecurityDescriptor ? &sa : nullptr
        );

        if (hPipe == INVALID_HANDLE_VALUE) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            continue;
        }

        BOOL connected = ConnectNamedPipe(hPipe, nullptr) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
        if (connected && m_isRunning) {
            std::thread clientThread(&NamedPipeServer::HandleClient, this, hPipe);
            clientThread.detach();
        } else {
            CloseHandle(hPipe);
        }
    }

    if (pSD) {
        LocalFree(pSD);
    }
}

void NamedPipeServer::HandleClient(HANDLE hPipe) {
    char buffer[4096] = { 0 };
    DWORD bytesRead = 0;

    while (m_isRunning && ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, nullptr) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        std::string request(buffer);

        if (request.find("CP_WAIT_FOR_AUTH") != std::string::npos) {
            std::unique_lock<std::mutex> lock(m_authMutex);
            m_authHasResult = false;
            m_authCv.wait_for(lock, std::chrono::seconds(25), [this] { return m_authHasResult || !m_isRunning; });

            PipeAuthPacket packet;
            ZeroMemory(&packet, sizeof(packet));
            packet.magic = PipeAuthPacket::MAGIC;

            if (m_authHasResult && m_authSuccess) {
                packet.status = 1;
                packet.cbUsername = static_cast<uint32_t>(m_authUsername.length());
                wcsncpy_s(packet.szUsername, m_authUsername.c_str(), _TRUNCATE);

                packet.cbDomain = static_cast<uint32_t>(m_authDomain.length());
                wcsncpy_s(packet.szDomain, m_authDomain.c_str(), _TRUNCATE);

                packet.cbPassword = static_cast<uint32_t>(m_authPassword.length());
                wcsncpy_s(packet.szPassword, m_authPassword.c_str(), _TRUNCATE);

                packet.cbPhoneName = static_cast<uint32_t>(m_authPhoneName.length());
                strncpy_s(packet.szPhoneName, m_authPhoneName.c_str(), _TRUNCATE);

                packet.cbPhoneId = static_cast<uint32_t>(m_authPhoneId.length());
                strncpy_s(packet.szPhoneId, m_authPhoneId.c_str(), _TRUNCATE);
            } else {
                packet.status = 0;
            }

            // Immediately scrub in-memory password copy in server
            SecureZeroMemory(const_cast<wchar_t*>(m_authPassword.data()), m_authPassword.size() * sizeof(wchar_t));
            m_authPassword.clear();

            DWORD bytesWritten = 0;
            WriteFile(hPipe, &packet, sizeof(packet), &bytesWritten, nullptr);
            FlushFileBuffers(hPipe);

            // Clean the stack packet memory
            SecureZeroMemory(&packet, sizeof(packet));
        } else {
            // General text commands (status query / lock trigger)
            std::string response = ProcessCommand(request) + "\n";
            DWORD bytesWritten = 0;
            WriteFile(hPipe, response.c_str(), static_cast<DWORD>(response.length()), &bytesWritten, nullptr);
            FlushFileBuffers(hPipe);
        }

        SecureZeroMemory(buffer, sizeof(buffer));
    }

    DisconnectNamedPipe(hPipe);
    CloseHandle(hPipe);
}

std::string NamedPipeServer::ProcessCommand(const std::string& requestJson) {
    if (requestJson.find("CP_GET_STATUS") != std::string::npos) {
        bool isLocked = SessionMonitor::Instance().IsLocked();
        auto phones = KeyStore::Instance().GetTrustedPhones();
        std::stringstream ss;
        ss << "{\"status\":\"OK\",\"isLocked\":" << (isLocked ? "true" : "false")
           << ",\"trustedPhonesCount\":" << phones.size() << "}";
        return ss.str();
    }

    if (requestJson.find("UI_TRIGGER_LOCK") != std::string::npos) {
        SessionMonitor::Instance().LockWorkStation();
        return "{\"status\":\"OK\"}";
    }

    return "{\"status\":\"OK\"}";
}

} // namespace AnshuBio
