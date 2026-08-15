#pragma once
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <string>
#include <vector>
#include <memory>
#include "../core/Models.h"

namespace AnshuBio {

enum class AppPage {
    Dashboard = 0,
    TrustedPhones = 1,
    Security = 2,
    Settings = 3,
    Logs = 4,
    About = 5
};

class NativeWin32Window {
public:
    static NativeWin32Window& Instance();

    bool Initialize(HINSTANCE hInstance, int nCmdShow);
    int RunMessageLoop();
    void Show();
    void Hide();
    void SwitchToPage(AppPage page);

    LRESULT HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    NativeWin32Window();
    ~NativeWin32Window();

    void CreateSidebar();
    void CreatePages();
    void UpdateDashboard();
    void UpdateTrustedPhones();
    void UpdateSecurity();
    void UpdateSettings();
    void UpdateLogs();
    void UpdateAbout();

    void SetupTrayIcon();
    void RemoveTrayIcon();
    void RefreshAll();

    HINSTANCE m_hInstance = nullptr;
    HWND m_hWnd = nullptr;
    AppPage m_currentPage = AppPage::Dashboard;

    // Sidebar buttons
    HWND m_hBtnNavDashboard = nullptr;
    HWND m_hBtnNavPhones = nullptr;
    HWND m_hBtnNavSecurity = nullptr;
    HWND m_hBtnNavSettings = nullptr;
    HWND m_hBtnNavLogs = nullptr;
    HWND m_hBtnNavAbout = nullptr;

    // Page containers / controls
    HWND m_hPageDashboard = nullptr;
    HWND m_hPagePhones = nullptr;
    HWND m_hPageSecurity = nullptr;
    HWND m_hPageSettings = nullptr;
    HWND m_hPageLogs = nullptr;
    HWND m_hPageAbout = nullptr;

    // Dashboard controls
    HWND m_hDashStatusBadge = nullptr;
    HWND m_hDashPcName = nullptr;
    HWND m_hDashOsInfo = nullptr;
    HWND m_hDashSessionState = nullptr;
    HWND m_hDashConnInfo = nullptr;
    HWND m_hDashPhonesSummary = nullptr;
    HWND m_hDashPairBtn = nullptr;
    HWND m_hDashLockBtn = nullptr;

    // Phones controls
    HWND m_hPhonesCountLabel = nullptr;
    HWND m_hPhoneCard1 = nullptr;
    HWND m_hPhoneCard2 = nullptr;
    HWND m_hPhone1Info = nullptr;
    HWND m_hPhone2Info = nullptr;
    HWND m_hPhone1RevokeBtn = nullptr;
    HWND m_hPhone2RevokeBtn = nullptr;
    HWND m_hPhone1RemoveBtn = nullptr;
    HWND m_hPhone2RemoveBtn = nullptr;
    HWND m_hPhonesPairBtn = nullptr;
    HWND m_hPhonesMaxWarning = nullptr;

    // Security controls
    HWND m_hSecDeviceCard = nullptr;
    HWND m_hSecCryptoCard = nullptr;
    HWND m_hSecVaultCard = nullptr;
    HWND m_hSecFpCard = nullptr;
    HWND m_hSecLastAuthCard = nullptr;

    // Settings controls
    HWND m_hChkStartWithWindows = nullptr;
    HWND m_hChkWifiEnabled = nullptr;
    HWND m_hChkBluetoothEnabled = nullptr;
    HWND m_hChkSoundEnabled = nullptr;
    HWND m_hChkVibrationEnabled = nullptr;
    HWND m_hBtnSaveSettings = nullptr;

    // Logs controls
    HWND m_hLogListView = nullptr;
    HWND m_hLogBox = nullptr;
    HWND m_hBtnRefreshLogs = nullptr;

    // Custom Dark Brushes & Fonts
    HBRUSH m_hBrushBgMain = nullptr;
    HBRUSH m_hBrushBgSidebar = nullptr;
    HBRUSH m_hBrushBgCard = nullptr;
    HBRUSH m_hBrushAccent = nullptr;
    HFONT m_hFontAppTitle = nullptr;
    HFONT m_hFontHeader = nullptr;
    HFONT m_hFontSubHeader = nullptr;
    HFONT m_hFontNormal = nullptr;
    HFONT m_hFontBold = nullptr;
    HFONT m_hFontMono = nullptr;
    HFONT m_hFontSmall = nullptr;

    NOTIFYICONDATAW m_nid{};
    bool m_isMinimizedToTray = false;
};

} // namespace AnshuBio
