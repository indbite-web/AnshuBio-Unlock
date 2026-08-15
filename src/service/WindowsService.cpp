#include "WindowsService.h"
#include "../core/Constants.h"
#include "../core/AuthCoordinator.h"
#include "../networking/wifi/WiFiServer.h"
#include "../networking/bluetooth/BluetoothServer.h"
#include "../storage/SecurityLogger.h"

namespace AnshuBio {

WindowsService& WindowsService::Instance() {
    static WindowsService s_instance;
    return s_instance;
}

WindowsService::WindowsService() {}

WindowsService::~WindowsService() {}

bool WindowsService::IsServiceRunning() {
    SC_HANDLE hSCM = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!hSCM) return false;

    SC_HANDLE hService = OpenServiceW(hSCM, L"AnshuBioUnlockService", SERVICE_QUERY_STATUS);
    if (!hService) {
        CloseServiceHandle(hSCM);
        return false;
    }

    SERVICE_STATUS status;
    bool running = false;
    if (QueryServiceStatus(hService, &status)) {
        running = (status.dwCurrentState == SERVICE_RUNNING || status.dwCurrentState == SERVICE_START_PENDING);
    }

    CloseServiceHandle(hService);
    CloseServiceHandle(hSCM);
    return running;
}

bool WindowsService::StartServiceInstance() {
    SC_HANDLE hSCM = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!hSCM) return false;

    SC_HANDLE hService = OpenServiceW(hSCM, L"AnshuBioUnlockService", SERVICE_START);
    if (!hService) {
        CloseServiceHandle(hSCM);
        return false;
    }

    BOOL started = StartServiceW(hService, 0, nullptr);
    CloseServiceHandle(hService);
    CloseServiceHandle(hSCM);
    return started == TRUE;
}

void WINAPI WindowsService::ServiceMain(DWORD argc, LPWSTR* argv) {
    (void)argc;
    (void)argv;
    Instance().m_statusHandle = RegisterServiceCtrlHandlerW(L"AnshuBioUnlockService", ServiceCtrlHandler);
    if (!Instance().m_statusHandle) return;

    Instance().m_serviceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    Instance().m_serviceStatus.dwCurrentState = SERVICE_START_PENDING;
    Instance().m_serviceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_POWEREVENT;
    Instance().ReportServiceStatus(SERVICE_START_PENDING, NO_ERROR, 3000);

    Instance().m_stopEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);

    // Initialize core authentication & networking components
    WiFiServer wifiServer(&AuthCoordinator::Instance());
    BluetoothServer btServer(&AuthCoordinator::Instance());

    AuthCoordinator::Instance().SetTransports(&wifiServer, &btServer);
    AuthCoordinator::Instance().Start();
    wifiServer.Start(Network::SERVER_PORT);
    btServer.Start();

    Instance().ReportServiceStatus(SERVICE_RUNNING, NO_ERROR, 0);
    SecurityLogger::Instance().Security("SERVICE_ACTIVE", "AnshuBio Unlock Background Windows Service is RUNNING");

    WaitForSingleObject(Instance().m_stopEvent, INFINITE);

    Instance().ReportServiceStatus(SERVICE_STOP_PENDING, NO_ERROR, 3000);

    wifiServer.Stop();
    btServer.Stop();
    AuthCoordinator::Instance().Stop();

    Instance().ReportServiceStatus(SERVICE_STOPPED, NO_ERROR, 0);
    SecurityLogger::Instance().Security("SERVICE_STOP", "AnshuBio Unlock Background Service cleanly terminated");
}

void WINAPI WindowsService::ServiceCtrlHandler(DWORD ctrlCode) {
    switch (ctrlCode) {
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:
        Instance().ReportServiceStatus(SERVICE_STOP_PENDING, NO_ERROR, 3000);
        if (Instance().m_stopEvent) {
            SetEvent(Instance().m_stopEvent);
        }
        break;
    case SERVICE_CONTROL_POWEREVENT:
        break;
    case SERVICE_CONTROL_INTERROGATE:
        break;
    default:
        break;
    }
}

void WindowsService::ReportServiceStatus(DWORD currentState, DWORD exitCode, DWORD waitHint) {
    static DWORD s_checkPoint = 1;
    m_serviceStatus.dwCurrentState = currentState;
    m_serviceStatus.dwWin32ExitCode = exitCode;
    m_serviceStatus.dwWaitHint = waitHint;

    if (currentState == SERVICE_START_PENDING) {
        m_serviceStatus.dwControlsAccepted = 0;
    } else {
        m_serviceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_POWEREVENT;
    }

    if (currentState == SERVICE_RUNNING || currentState == SERVICE_STOPPED) {
        m_serviceStatus.dwCheckPoint = 0;
    } else {
        m_serviceStatus.dwCheckPoint = s_checkPoint++;
    }

    SetServiceStatus(m_statusHandle, &m_serviceStatus);
}

bool WindowsService::InstallService() {
    SC_HANDLE hSCM = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
    if (!hSCM) return false;

    wchar_t path[MAX_PATH];
    if (!GetModuleFileNameW(nullptr, path, MAX_PATH)) {
        CloseServiceHandle(hSCM);
        return false;
    }

    std::wstring serviceCmd = L"\"" + std::wstring(path) + L"\" --service";

    SC_HANDLE hService = CreateServiceW(
        hSCM,
        L"AnshuBioUnlockService",
        L"AnshuBio Unlock Background Service",
        SERVICE_ALL_ACCESS,
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_AUTO_START,
        SERVICE_ERROR_NORMAL,
        serviceCmd.c_str(),
        nullptr, nullptr, nullptr, nullptr, nullptr
    );

    if (!hService) {
        CloseServiceHandle(hSCM);
        return false;
    }

    // Set Service Description
    SERVICE_DESCRIPTIONW sd;
    wchar_t desc[] = L"Coordinates local biometric authentication from trusted Android phones to unlock Windows workstation.";
    sd.lpDescription = desc;
    ChangeServiceConfig2W(hService, SERVICE_CONFIG_DESCRIPTION, &sd);

    // Set Service Failure Actions (Automatic restart on crash/failure)
    SC_ACTION actions[3];
    actions[0].Type = SC_ACTION_RESTART;
    actions[0].Delay = 3000; // 3 seconds
    actions[1].Type = SC_ACTION_RESTART;
    actions[1].Delay = 5000; // 5 seconds
    actions[2].Type = SC_ACTION_NONE;
    actions[2].Delay = 0;

    SERVICE_FAILURE_ACTIONSW sfa;
    ZeroMemory(&sfa, sizeof(sfa));
    sfa.dwResetPeriod = 86400; // 1 day
    sfa.cActions = 3;
    sfa.lpsaActions = actions;
    ChangeServiceConfig2W(hService, SERVICE_CONFIG_FAILURE_ACTIONS, &sfa);

    CloseServiceHandle(hService);
    CloseServiceHandle(hSCM);
    SecurityLogger::Instance().Info("SERVICE", "Installed AnshuBio Unlock Windows Service into SCM with auto-recovery");
    return true;
}

bool WindowsService::UninstallService() {
    SC_HANDLE hSCM = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
    if (!hSCM) return false;

    SC_HANDLE hService = OpenServiceW(hSCM, L"AnshuBioUnlockService", SERVICE_STOP | DELETE);
    if (!hService) {
        CloseServiceHandle(hSCM);
        return false;
    }

    SERVICE_STATUS status;
    ControlService(hService, SERVICE_CONTROL_STOP, &status);
    BOOL deleted = DeleteService(hService);

    CloseServiceHandle(hService);
    CloseServiceHandle(hSCM);
    SecurityLogger::Instance().Info("SERVICE", "Uninstalled AnshuBio Unlock Windows Service from SCM");
    return deleted == TRUE;
}

bool WindowsService::RunService() {
    SERVICE_TABLE_ENTRYW serviceTable[] = {
        { const_cast<LPWSTR>(L"AnshuBioUnlockService"), ServiceMain },
        { nullptr, nullptr }
    };
    return StartServiceCtrlDispatcherW(serviceTable) == TRUE;
}

} // namespace AnshuBio
