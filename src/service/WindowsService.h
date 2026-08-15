#pragma once
#include <windows.h>
#include <string>

namespace AnshuBio {

class WindowsService {
public:
    static WindowsService& Instance();

    static void WINAPI ServiceMain(DWORD argc, LPWSTR* argv);
    static void WINAPI ServiceCtrlHandler(DWORD ctrlCode);

    bool InstallService();
    bool UninstallService();
    bool RunService();

    static bool IsServiceRunning();
    static bool StartServiceInstance();

private:
    WindowsService();
    ~WindowsService();

    void ReportServiceStatus(DWORD currentState, DWORD exitCode, DWORD waitHint);

    SERVICE_STATUS m_serviceStatus{};
    SERVICE_STATUS_HANDLE m_statusHandle = nullptr;
    HANDLE m_stopEvent = nullptr;
};

} // namespace AnshuBio
