#pragma once
#include <windows.h>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>

namespace AnshuBio {

class NamedPipeServer {
public:
    static NamedPipeServer& Instance();

    bool Start();
    void Stop();

    void NotifyAuthResolved(bool success, const std::string& phoneId, const std::string& phoneName,
                            const std::wstring& username, const std::wstring& domain, const std::wstring& password);

private:
    NamedPipeServer();
    ~NamedPipeServer();

    void ServerLoop();
    void HandleClient(HANDLE hPipe);
    std::string ProcessCommand(const std::string& requestJson);

    std::wstring m_pipeName;
    std::atomic<bool> m_isRunning{false};
    std::thread m_serverThread;

    std::mutex m_authMutex;
    std::condition_variable m_authCv;
    bool m_authHasResult = false;
    bool m_authSuccess = false;
    std::string m_authPhoneId;
    std::string m_authPhoneName;
    std::wstring m_authUsername;
    std::wstring m_authDomain;
    std::wstring m_authPassword;
};

} // namespace AnshuBio
