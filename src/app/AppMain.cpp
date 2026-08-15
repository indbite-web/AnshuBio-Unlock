#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <string>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <memory>
#include <ctime>

#include "../core/Constants.h"
#include "../core/AuthCoordinator.h"
#include "../storage/KeyStore.h"
#include "../storage/SecurityLogger.h"
#include "../service/WindowsService.h"
#include "../networking/wifi/WiFiServer.h"
#include "../networking/bluetooth/BluetoothServer.h"
#include "../session/SessionMonitor.h"
#include "../ui/NativeWin32Window.h"

namespace AnshuBio {

static void LogStartupStage(const std::string& stage, const std::string& details = "") {
    wchar_t localAppData[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, localAppData))) {
        std::wstring dir = std::wstring(localAppData) + L"\\AnshuBio\\logs";
        CreateDirectoryW((std::wstring(localAppData) + L"\\AnshuBio").c_str(), nullptr);
        CreateDirectoryW(dir.c_str(), nullptr);

        char pathA[MAX_PATH];
        WideCharToMultiByte(CP_UTF8, 0, localAppData, -1, pathA, MAX_PATH, nullptr, nullptr);
        std::string logPath = std::string(pathA) + "\\AnshuBio\\logs\\startup.log";

        std::ofstream ofs(logPath, std::ios::app);
        if (ofs.is_open()) {
            auto now = std::chrono::system_clock::now();
            auto timeT = std::chrono::system_clock::to_time_t(now);
            std::tm tm{};
#if defined(_MSC_VER)
            localtime_s(&tm, &timeT);
#else
            std::tm* pTm = std::localtime(&timeT);
            if (pTm) tm = *pTm;
#endif
            ofs << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << " [STARTUP] [" << stage << "] " << details << "\n";
            ofs.close();
        }
    }
}

} // namespace AnshuBio

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    (void)hPrevInstance;
    (void)lpCmdLine;

    using namespace AnshuBio;

    LogStartupStage("ApplicationStart", "AnshuBio Unlock v1.0.0 (Native Win32 x64 GUI)");

    // 1. Check command line arguments for service actions
    int argc = 0;
    LPWSTR* argvW = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argvW) {
        for (int i = 1; i < argc; ++i) {
            std::wstring arg = argvW[i];
            if (arg == L"--service") {
                LogStartupStage("ServiceMode", "Launching Windows Service Dispatcher");
                LocalFree(argvW);
                return WindowsService::Instance().RunService() ? 0 : 1;
            }
            if (arg == L"--install") {
                LogStartupStage("ServiceInstall", "Installing AnshuBioUnlockService");
                bool ok = WindowsService::Instance().InstallService();
                LocalFree(argvW);
                return ok ? 0 : 1;
            }
            if (arg == L"--uninstall") {
                LogStartupStage("ServiceUninstall", "Uninstalling AnshuBioUnlockService");
                bool ok = WindowsService::Instance().UninstallService();
                LocalFree(argvW);
                return ok ? 0 : 1;
            }
        }
    }

    // 2. Enforce Single Instance
    LogStartupStage("SingleInstanceCheck");
    HANDLE hMutex = CreateMutexW(nullptr, TRUE, L"Global\\AnshuBioUnlockSingleInstanceMutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        LogStartupStage("AlreadyRunning", "Bringing existing instance to foreground");
        if (hMutex) CloseHandle(hMutex);
        HWND hWndExisting = FindWindowW(L"AnshuBioUnlockMainWindowClass", L"AnshuBio Unlock");
        if (hWndExisting) {
            ShowWindow(hWndExisting, SW_RESTORE);
            SetForegroundWindow(hWndExisting);
        }
        if (argvW) LocalFree(argvW);
        return 0;
    }

    // 3. Initialize Storage
    LogStartupStage("StorageInitialized", "DPAPI Vault & KeyStore ready");
    KeyStore::Instance().Load();

    // 4. Determine Service vs Standalone Mode
    LogStartupStage("ServiceConnectionAttempt");
    bool serviceActive = WindowsService::IsServiceRunning();
    std::unique_ptr<WiFiServer> localWifiServer;
    std::unique_ptr<BluetoothServer> localBtServer;

    if (serviceActive) {
        LogStartupStage("ServiceConnected", "Windows Service running. GUI connected via IPC (no duplicate socket binding).");
        SecurityLogger::Instance().Info("APP", "Connected to active Windows Service.");
    } else {
        LogStartupStage("StandaloneMode", "Windows Service inactive. Starting integrated server stack.");
        SecurityLogger::Instance().Info("APP", "Windows Service inactive. Starting local servers.");
        localWifiServer = std::make_unique<WiFiServer>(&AuthCoordinator::Instance());
        localBtServer = std::make_unique<BluetoothServer>(&AuthCoordinator::Instance());

        AuthCoordinator::Instance().SetTransports(localWifiServer.get(), localBtServer.get());
        AuthCoordinator::Instance().Start();
        localWifiServer->Start(Network::SERVER_PORT);
        localBtServer->Start();
    }

    // 5. Initialize Session Monitor
    SessionMonitor::Instance().Start();

    // 6. Initialize Native UI
    LogStartupStage("UIInitialization", "Creating Native Win32 window controls");
    bool startInBackground = false;
    if (argvW) {
        for (int i = 1; i < argc; ++i) {
            std::wstring arg = argvW[i];
            if (arg == L"--background" || arg == L"-b") {
                startInBackground = true;
                break;
            }
        }
        LocalFree(argvW);
    }

    int showFlag = startInBackground ? SW_HIDE : nCmdShow;
    if (showFlag == 0 && !startInBackground) showFlag = SW_SHOW;

    if (!NativeWin32Window::Instance().Initialize(hInstance, showFlag)) {
        LogStartupStage("UIError", "Failed to create main window");
        if (hMutex) {
            ReleaseMutex(hMutex);
            CloseHandle(hMutex);
        }
        return 1;
    }

    LogStartupStage("MainWindowCreated", "GUI Window visible and responsive");
    LogStartupStage("ApplicationExecStarted", "Entering Win32 message pump");

    // 7. Run Message Loop
    int exitCode = NativeWin32Window::Instance().RunMessageLoop();

    LogStartupStage("ApplicationShutdown", "Clean exit code " + std::to_string(exitCode));

    // 8. Clean Shutdown
    SessionMonitor::Instance().Stop();
    if (!serviceActive) {
        if (localWifiServer) localWifiServer->Stop();
        if (localBtServer) localBtServer->Stop();
        AuthCoordinator::Instance().Stop();
    }

    if (hMutex) {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
    }

    return exitCode;
}

// Fallback entrypoint when invoked as console
int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    return WinMain(GetModuleHandleW(nullptr), nullptr, GetCommandLineA(), SW_SHOW);
}
