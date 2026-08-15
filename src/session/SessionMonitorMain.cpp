#include "SessionMonitor.h"
#include "../storage/SecurityLogger.h"
#include <windows.h>
#include <iostream>

using namespace AnshuBio;

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    SecurityLogger::Instance().Info("SESSION_MON", "AnshuBio Session Monitor standalone process started.");

    SessionMonitor::Instance().SetLockCallback([]() {
        SecurityLogger::Instance().Security("SESSION_LOCKED", "Session locked event received by standalone monitor");
    });

    SessionMonitor::Instance().SetUnlockCallback([]() {
        SecurityLogger::Instance().Security("SESSION_UNLOCKED", "Session unlocked event received by standalone monitor");
    });

    if (!SessionMonitor::Instance().Start()) {
        std::cerr << "Failed to initialize WTS session notifications.\n";
        return 1;
    }

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    SessionMonitor::Instance().Stop();
    return 0;
}
