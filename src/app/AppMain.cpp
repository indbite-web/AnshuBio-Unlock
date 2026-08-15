#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <string>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <memory>
#include <ctime>
#include <exception>

#include "../core/Constants.h"
#include "../core/AuthCoordinator.h"
#include "../storage/KeyStore.h"
#include "../storage/SecurityLogger.h"
#include "../service/WindowsService.h"
#include "../session/SessionMonitor.h"
#include "../ui/NativeWin32Window.h"

namespace AnshuBio {

static void LogStartupStage(const std::string& stage, const std::string& details = "") {
    try {
        wchar_t localAppData[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, localAppData))) {
            std::wstring baseDir = std::wstring(localAppData) + L"\\AnshuBio";
            std::wstring dir = baseDir + L"\\logs";
            CreateDirectoryW(baseDir.c_str(), nullptr);
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
                ofs.flush();
                ofs.close();
            }
        }
    } catch (...) {}
}

} // namespace AnshuBio

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    (void)hPrevInstance;
    (void)lpCmdLine;

    using namespace AnshuBio;

    try {
        LogStartupStage("1.ProcessStarted", "AnshuBio Unlock v1.0.0 (Native Win32 GUI x64)");

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

        // 2. Single Instance Check (Per-User Session)
        LogStartupStage("2.SingleInstanceCheck");
        HANDLE hMutex = CreateMutexW(nullptr, TRUE, L"Local\\AnshuBioUnlockDesktopSessionMutex");
        if (hMutex != nullptr && GetLastError() == ERROR_ALREADY_EXISTS) {
            HWND hWndExisting = FindWindowW(L"AnshuBioUnlockMainWindowClass", L"AnshuBio Unlock");
            if (hWndExisting && IsWindow(hWndExisting)) {
                LogStartupStage("AlreadyRunning", "Bringing existing active instance to foreground");
                ShowWindow(hWndExisting, SW_RESTORE);
                SetForegroundWindow(hWndExisting);
                CloseHandle(hMutex);
                if (argvW) LocalFree(argvW);
                return 0;
            }
            // If mutex exists but window was closed/orphaned, continue execution
            LogStartupStage("MutexOrphaned", "Existing window not found, continuing startup");
        }

        // 3. Storage Initialized
        LogStartupStage("3.StorageInitialized", "Initializing DPAPI Vault & KeyStore");
        KeyStore::Instance().Load();

        // 4. Service IPC Initialized
        LogStartupStage("4.ServiceIpcInitialized", "Checking Windows Service Status");
        bool serviceActive = WindowsService::IsServiceRunning();
        if (serviceActive) {
            LogStartupStage("ServiceConnected", "Windows Service running. GUI connected via IPC channel.");
            SecurityLogger::Instance().Info("APP", "Connected to active Windows Service.");
        } else {
            LogStartupStage("ServiceUnavailable", "Windows Service inactive. GUI operating in Standalone Security Mode.");
            SecurityLogger::Instance().Info("APP", "Windows Service inactive. GUI in standalone mode.");
        }

        // 5. Session Monitor Initialized
        LogStartupStage("5.SessionMonitorStarted");
        SessionMonitor::Instance().Start();

        // 6. UI Initialized
        LogStartupStage("6.UIInitialized", "Creating Native Win32 window controls");
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

        // 7. Main Window Created
        if (!NativeWin32Window::Instance().Initialize(hInstance, showFlag)) {
            LogStartupStage("UIError", "Failed to create main window");
            if (hMutex) {
                ReleaseMutex(hMutex);
                CloseHandle(hMutex);
            }
            return 1;
        }

        LogStartupStage("7.MainWindowCreated", "GUI Window visible, responsive and painted");
        LogStartupStage("8.EventLoopStarted", "Entering Win32 message pump");

        // 8. Event Loop Started
        int exitCode = NativeWin32Window::Instance().RunMessageLoop();

        LogStartupStage("9.ApplicationShutdown", "Clean exit code " + std::to_string(exitCode));

        // Clean Shutdown
        SessionMonitor::Instance().Stop();
        if (hMutex) {
            ReleaseMutex(hMutex);
            CloseHandle(hMutex);
        }

        return exitCode;

    } catch (const std::exception& ex) {
        LogStartupStage("CRASH_EXCEPTION", std::string("Unhandled exception: ") + ex.what());
        MessageBoxA(nullptr, ex.what(), "AnshuBio Unlock — Startup Exception", MB_ICONERROR | MB_OK);
        return 1;
    } catch (...) {
        LogStartupStage("CRASH_UNKNOWN", "Unhandled unknown exception");
        MessageBoxW(nullptr, L"An unexpected error occurred during startup.", L"AnshuBio Unlock — Fatal Error", MB_ICONERROR | MB_OK);
        return 1;
    }
}

// Fallback entrypoint when invoked as console
int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    return WinMain(GetModuleHandleW(nullptr), nullptr, GetCommandLineA(), SW_SHOW);
}
