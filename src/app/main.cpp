#include <QApplication>
#include <windows.h>
#include <memory>
#include "../ui/MainWindow.h"
#include "../service/WindowsService.h"
#include "../core/AuthCoordinator.h"
#include "../networking/wifi/WiFiServer.h"
#include "../networking/bluetooth/BluetoothServer.h"
#include "../storage/SecurityLogger.h"
#include "../core/Constants.h"

int main(int argc, char* argv[]) {
    // 1. Check for command line flags for service management
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--service") {
            return AnshuBio::WindowsService::Instance().RunService() ? 0 : 1;
        }
        if (arg == "--install") {
            return AnshuBio::WindowsService::Instance().InstallService() ? 0 : 1;
        }
        if (arg == "--uninstall") {
            return AnshuBio::WindowsService::Instance().UninstallService() ? 0 : 1;
        }
    }

    // 2. Enforce Single Instance
    HANDLE hMutex = CreateMutexW(nullptr, TRUE, L"Global\\AnshuBioUnlockSingleInstanceMutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        if (hMutex) CloseHandle(hMutex);
        HWND hWndExisting = FindWindowW(nullptr, L"AnshuBio Unlock");
        if (hWndExisting) {
            ShowWindow(hWndExisting, SW_RESTORE);
            SetForegroundWindow(hWndExisting);
        }
        return 0;
    }

    // 3. Initialize Qt Application
    QApplication app(argc, argv);
    app.setApplicationName(AnshuBio::Product::NAME);
    app.setApplicationDisplayName(AnshuBio::Product::NAME);
    app.setOrganizationName(AnshuBio::Product::PUBLISHER);
    app.setOrganizationDomain(AnshuBio::Product::APP_ID);

    // 4. Determine Service vs Standalone Architecture Ownership
    bool serviceActive = AnshuBio::WindowsService::IsServiceRunning();
    std::unique_ptr<AnshuBio::WiFiServer> localWifiServer;
    std::unique_ptr<AnshuBio::BluetoothServer> localBtServer;

    if (serviceActive) {
        AnshuBio::SecurityLogger::Instance().Info("APP", "Windows Service is active. GUI connected to service IPC without duplicate network bindings.");
    } else {
        AnshuBio::SecurityLogger::Instance().Info("APP", "Windows Service not running standalone. Launching integrated network stack.");
        localWifiServer = std::make_unique<AnshuBio::WiFiServer>(&AnshuBio::AuthCoordinator::Instance());
        localBtServer = std::make_unique<AnshuBio::BluetoothServer>(&AnshuBio::AuthCoordinator::Instance());

        AnshuBio::AuthCoordinator::Instance().SetTransports(localWifiServer.get(), localBtServer.get());
        AnshuBio::AuthCoordinator::Instance().Start();
        localWifiServer->Start(AnshuBio::Network::SERVER_PORT);
        localBtServer->Start();
    }

    // 5. Initialize Main GUI
    AnshuBio::MainWindow mainWindow;

    bool startInBackground = false;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--background" || arg == "-b") {
            startInBackground = true;
            break;
        }
    }

    if (!startInBackground) {
        mainWindow.show();
    }

    AnshuBio::SecurityLogger::Instance().Info("APP", "AnshuBio Unlock Desktop UI active");

    int exitCode = app.exec();

    // 6. Clean Shutdown
    if (!serviceActive) {
        if (localWifiServer) localWifiServer->Stop();
        if (localBtServer) localBtServer->Stop();
        AnshuBio::AuthCoordinator::Instance().Stop();
    }

    if (hMutex) {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
    }

    return exitCode;
}
