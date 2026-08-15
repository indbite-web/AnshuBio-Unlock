#include <windows.h>
#include <string>
#include "../service/WindowsService.h"

#if defined(ANSHUBIO_USE_QT)
#include <QApplication>
#include "../core/Constants.h"
#include "../ui/MainWindow.h"
#else
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
#endif

int main(int argc, char* argv[]) {
    using namespace AnshuBio;

    // 1. Check for command line flags for service management
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--service") {
            return WindowsService::Instance().RunService() ? 0 : 1;
        }
        if (arg == "--install") {
            return WindowsService::Instance().InstallService() ? 0 : 1;
        }
        if (arg == "--uninstall") {
            return WindowsService::Instance().UninstallService() ? 0 : 1;
        }
    }

    // 2. Enforce Single Instance
    HANDLE hMutex = CreateMutexW(nullptr, TRUE, L"Local\\AnshuBioUnlockDesktopSessionMutex");
    if (hMutex != nullptr && GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND hWndExisting = FindWindowW(L"AnshuBioUnlockMainWindowClass", L"AnshuBio Unlock");
        if (hWndExisting && IsWindow(hWndExisting)) {
            ShowWindow(hWndExisting, SW_RESTORE);
            SetForegroundWindow(hWndExisting);
            CloseHandle(hMutex);
            return 0;
        }
    }

#if defined(ANSHUBIO_USE_QT)
    QApplication app(argc, argv);
    app.setApplicationName(Product::NAME);
    app.setApplicationDisplayName(Product::NAME);
    app.setOrganizationName(Product::PUBLISHER);
    app.setOrganizationDomain(Product::APP_ID);

    MainWindow window;
    window.show();
    int res = app.exec();
    if (hMutex) {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
    }
    return res;
#else
    (void)argc;
    (void)argv;
    return WinMain(GetModuleHandleW(nullptr), nullptr, GetCommandLineA(), SW_SHOW);
#endif
}
