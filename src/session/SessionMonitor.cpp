#include "SessionMonitor.h"
#include "../storage/SecurityLogger.h"
#include <thread>

#pragma comment(lib, "wtsapi32.lib")
#pragma comment(lib, "user32.lib")

namespace AnshuBio {

static LRESULT CALLBACK SessionMonitorWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    return SessionMonitor::Instance().HandleMessage(hWnd, msg, wParam, lParam);
}

DWORD WINAPI SessionMonitor::SessionMonitorThreadProc(LPVOID param) {
    if (param) {
        static_cast<SessionMonitor*>(param)->MessageThreadProc();
    }
    return 0;
}

SessionMonitor& SessionMonitor::Instance() {
    static SessionMonitor s_instance;
    return s_instance;
}

SessionMonitor::SessionMonitor() {}

SessionMonitor::~SessionMonitor() {
    Stop();
}

bool SessionMonitor::Start() {
    if (m_isRunning) return true;
    m_isRunning = true;

    m_hThread = CreateThread(nullptr, 0, SessionMonitorThreadProc, this, 0, nullptr);
    return (m_hThread != nullptr);
}

void SessionMonitor::Stop() {
    if (!m_isRunning) return;
    m_isRunning = false;

    if (m_hWnd) {
        PostMessageW(m_hWnd, WM_CLOSE, 0, 0);
    }
    if (m_hThread) {
        WaitForSingleObject(m_hThread, 1000);
        CloseHandle(m_hThread);
        m_hThread = nullptr;
    }
}

void SessionMonitor::MessageThreadProc() {
    WNDCLASSEXW wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = SessionMonitorWndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"AnshuBioSessionMonitorClass";

    RegisterClassExW(&wc);

    m_hWnd = CreateWindowExW(
        0,
        wc.lpszClassName,
        L"AnshuBioSessionMonitorWindow",
        0, 0, 0, 0, 0,
        HWND_MESSAGE,
        nullptr,
        wc.hInstance,
        nullptr
    );

    if (!m_hWnd) {
        SecurityLogger::Instance().Error("SESSION", "Failed to create Win32 session notification window");
        m_isRunning = false;
        return;
    }

    WTSRegisterSessionNotification(m_hWnd, NOTIFY_FOR_THIS_SESSION);
    SecurityLogger::Instance().Info("SESSION", "Registered real Windows WTS Session Notification listener (Native Win32)");

    MSG msg;
    while (m_isRunning && GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (m_hWnd) {
        WTSUnRegisterSessionNotification(m_hWnd);
        DestroyWindow(m_hWnd);
        m_hWnd = nullptr;
    }
}

LRESULT SessionMonitor::HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_WTSSESSION_CHANGE: {
        switch (wParam) {
        case WTS_SESSION_LOCK: {
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_isLocked = true;
                m_currentState = SessionState::Locked;
            }
            SecurityLogger::Instance().Security("SESSION_LOCKED", "Captured real Windows OS session event: WTS_SESSION_LOCK");
            if (m_onLock) m_onLock();
            break;
        }
        case WTS_SESSION_UNLOCK: {
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_isLocked = false;
                m_currentState = SessionState::RunningUnlocked;
            }
            SecurityLogger::Instance().Security("SESSION_UNLOCKED", "Captured real Windows OS session event: WTS_SESSION_UNLOCK");
            if (m_onUnlock) m_onUnlock();
            break;
        }
        case WTS_SESSION_LOGON: {
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_isLocked = false;
                m_currentState = SessionState::RunningUnlocked;
            }
            SecurityLogger::Instance().Security("SESSION_LOGON", "Captured real Windows OS session event: WTS_SESSION_LOGON");
            if (m_onLogon) m_onLogon();
            break;
        }
        case WTS_SESSION_LOGOFF: {
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_isLocked = true;
                m_currentState = SessionState::Locked;
            }
            SecurityLogger::Instance().Security("SESSION_LOGOFF", "Captured real Windows OS session event: WTS_SESSION_LOGOFF");
            if (m_onLogoff) m_onLogoff();
            break;
        }
        }
        return 0;
    }

    case WM_POWERBROADCAST: {
        if (wParam == PBT_APMSUSPEND) {
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_isSleeping = true;
                m_currentState = SessionState::Sleep;
            }
            SecurityLogger::Instance().Info("POWER", "System entering sleep / suspend mode (PBT_APMSUSPEND)");
            if (m_onSleep) m_onSleep();
        } else if (wParam == PBT_APMRESUMESUSPEND || wParam == PBT_APMRESUMEAUTOMATIC) {
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_isSleeping = false;
                m_isLocked = true;
                m_currentState = SessionState::Locked;
            }
            SecurityLogger::Instance().Info("POWER", "System resumed from sleep / wake (PBT_APMRESUMESUSPEND)");
            if (m_onWake) m_onWake();
        }
        return 0;
    }

    case WM_DESTROY: {
        PostQuitMessage(0);
        return 0;
    }
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

bool SessionMonitor::IsLocked() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_isLocked;
}

bool SessionMonitor::IsSleeping() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_isSleeping;
}

SessionState SessionMonitor::GetCurrentState() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_currentState;
}

bool SessionMonitor::LockWorkStation() {
    BOOL result = ::LockWorkStation();
    if (result) {
        SecurityLogger::Instance().Security("MANUAL_LOCK", "Executed native Win32 LockWorkStation() API");
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_isLocked = true;
            m_currentState = SessionState::Locked;
        }
        if (m_onLock) m_onLock();
        return true;
    }
    return false;
}

void SessionMonitor::SetLockCallback(std::function<void()> cb) { m_onLock = cb; }
void SessionMonitor::SetUnlockCallback(std::function<void()> cb) { m_onUnlock = cb; }
void SessionMonitor::SetLogonCallback(std::function<void()> cb) { m_onLogon = cb; }
void SessionMonitor::SetLogoffCallback(std::function<void()> cb) { m_onLogoff = cb; }
void SessionMonitor::SetSleepCallback(std::function<void()> cb) { m_onSleep = cb; }
void SessionMonitor::SetWakeCallback(std::function<void()> cb) { m_onWake = cb; }

} // namespace AnshuBio
