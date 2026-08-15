#pragma once
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <string>
#include <vector>
#include <memory>
#include "../core/Models.h"

namespace AnshuBio {

class NativeWin32Window {
public:
    static NativeWin32Window& Instance();

    bool Initialize(HINSTANCE hInstance, int nCmdShow);
    int RunMessageLoop();
    void Show();
    void Hide();

    LRESULT HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    NativeWin32Window();
    ~NativeWin32Window();

    void CreateControls();
    void UpdateDashboard();
    void UpdateLogs();
    void RefreshPhoneList();
    void SetupTrayIcon();
    void RemoveTrayIcon();

    HINSTANCE m_hInstance = nullptr;
    HWND m_hWnd = nullptr;
    HWND m_hTabControl = nullptr;
    HWND m_hStatusLabel = nullptr;
    HWND m_hPcNameLabel = nullptr;
    HWND m_hFingerprintLabel = nullptr;
    HWND m_hPhoneList = nullptr;
    HWND m_hLogBox = nullptr;
    HWND m_hPairBtn = nullptr;
    HWND m_hLockBtn = nullptr;
    HWND m_hPairCodeLabel = nullptr;

    NOTIFYICONDATAW m_nid{};
    bool m_isMinimizedToTray = false;
};

} // namespace AnshuBio
