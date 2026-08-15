#pragma once
#include <windows.h>
#include <wtsapi32.h>
#include <functional>
#include <mutex>
#include "../core/Constants.h"

namespace AnshuBio {

class SessionMonitor {
public:
    static SessionMonitor& Instance();

    bool Start();
    void Stop();

    bool IsLocked() const;
    bool IsSleeping() const;
    SessionState GetCurrentState() const;

    bool LockWorkStation();

    // Callbacks
    void SetLockCallback(std::function<void()> cb);
    void SetUnlockCallback(std::function<void()> cb);
    void SetLogonCallback(std::function<void()> cb);
    void SetLogoffCallback(std::function<void()> cb);
    void SetSleepCallback(std::function<void()> cb);
    void SetWakeCallback(std::function<void()> cb);

    // Internal window proc handler
    LRESULT HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    SessionMonitor();
    ~SessionMonitor();

    static DWORD WINAPI SessionMonitorThreadProc(LPVOID param);
    void MessageThreadProc();

    HWND m_hWnd = nullptr;
    HANDLE m_hThread = nullptr;
    volatile bool m_isRunning = false;
    mutable std::mutex m_mutex;

    SessionState m_currentState = SessionState::RunningUnlocked;
    bool m_isLocked = false;
    bool m_isSleeping = false;

    std::function<void()> m_onLock;
    std::function<void()> m_onUnlock;
    std::function<void()> m_onLogon;
    std::function<void()> m_onLogoff;
    std::function<void()> m_onSleep;
    std::function<void()> m_onWake;
};

} // namespace AnshuBio
