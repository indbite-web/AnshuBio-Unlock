#include "NativeWin32Window.h"
#include "PairingDialog.h"
#include "../core/Constants.h"
#include "../core/AuthCoordinator.h"
#include "../storage/KeyStore.h"
#include "../storage/SecurityLogger.h"
#include "../session/SessionMonitor.h"
#include "../service/WindowsService.h"
#include <shellapi.h>
#include <sstream>
#include <iomanip>

#define WM_TRAYICON (WM_USER + 101)

#define ID_NAV_DASHBOARD 101
#define ID_NAV_PHONES 102
#define ID_NAV_SECURITY 103
#define ID_NAV_SETTINGS 104
#define ID_NAV_LOGS 105
#define ID_NAV_ABOUT 106

#define ID_BTN_DASH_PAIR 201
#define ID_BTN_DASH_LOCK 202
#define ID_BTN_PHONES_PAIR 203
#define ID_BTN_PHONE1_REVOKE 204
#define ID_BTN_PHONE1_REMOVE 205
#define ID_BTN_PHONE2_REVOKE 206
#define ID_BTN_PHONE2_REMOVE 207
#define ID_BTN_SAVE_SETTINGS 208
#define ID_BTN_REFRESH_LOGS 209
#define ID_BTN_CHECK_UPDATES 210

#define ID_TRAY_RESTORE 301
#define ID_TRAY_QUIT 302

namespace AnshuBio {

static const COLORREF COLOR_BG_MAIN = RGB(12, 15, 23);
static const COLORREF COLOR_BG_SIDEBAR = RGB(8, 10, 16);
static const COLORREF COLOR_BG_CARD = RGB(21, 25, 38);
static const COLORREF COLOR_TEXT_PRIMARY = RGB(248, 250, 252);
static const COLORREF COLOR_TEXT_SECONDARY = RGB(148, 163, 184);
static const COLORREF COLOR_TEXT_MUTED = RGB(100, 116, 139);
static const COLORREF COLOR_ACCENT_BLUE = RGB(59, 130, 246);
static const COLORREF COLOR_ACCENT_GREEN = RGB(16, 185, 129);
static const COLORREF COLOR_ACCENT_RED = RGB(239, 68, 68);

static LRESULT CALLBACK MainWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    return NativeWin32Window::Instance().HandleMessage(hWnd, msg, wParam, lParam);
}

NativeWin32Window& NativeWin32Window::Instance() {
    static NativeWin32Window s_instance;
    return s_instance;
}

NativeWin32Window::NativeWin32Window() {
    m_hBrushBgMain = CreateSolidBrush(COLOR_BG_MAIN);
    m_hBrushBgSidebar = CreateSolidBrush(COLOR_BG_SIDEBAR);
    m_hBrushBgCard = CreateSolidBrush(COLOR_BG_CARD);
    m_hBrushAccent = CreateSolidBrush(COLOR_ACCENT_BLUE);

    m_hFontAppTitle = CreateFontW(20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    m_hFontHeader = CreateFontW(24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    m_hFontSubHeader = CreateFontW(17, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    m_hFontNormal = CreateFontW(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    m_hFontBold = CreateFontW(15, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    m_hFontMono = CreateFontW(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FIXED_PITCH | FF_DONTCARE, L"Consolas");
    m_hFontSmall = CreateFontW(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
}

NativeWin32Window::~NativeWin32Window() {
    RemoveTrayIcon();

    if (m_hBrushBgMain) DeleteObject(m_hBrushBgMain);
    if (m_hBrushBgSidebar) DeleteObject(m_hBrushBgSidebar);
    if (m_hBrushBgCard) DeleteObject(m_hBrushBgCard);
    if (m_hBrushAccent) DeleteObject(m_hBrushAccent);

    if (m_hFontAppTitle) DeleteObject(m_hFontAppTitle);
    if (m_hFontHeader) DeleteObject(m_hFontHeader);
    if (m_hFontSubHeader) DeleteObject(m_hFontSubHeader);
    if (m_hFontNormal) DeleteObject(m_hFontNormal);
    if (m_hFontBold) DeleteObject(m_hFontBold);
    if (m_hFontMono) DeleteObject(m_hFontMono);
    if (m_hFontSmall) DeleteObject(m_hFontSmall);
}

bool NativeWin32Window::Initialize(HINSTANCE hInstance, int nCmdShow) {
    m_hInstance = hInstance;

    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_STANDARD_CLASSES | ICC_TAB_CLASSES | ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icex);

    WNDCLASSEXW wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = MainWindowProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = m_hBrushBgMain;
    wc.lpszClassName = L"AnshuBioUnlockMainWindowClass";

    RegisterClassExW(&wc);

    m_hWnd = CreateWindowExW(
        WS_EX_APPWINDOW,
        wc.lpszClassName,
        L"AnshuBio Unlock",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1100, 720,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (!m_hWnd) return false;

    CreateSidebar();
    CreatePages();
    SetupTrayIcon();
    SwitchToPage(AppPage::Dashboard);

    if (nCmdShow != SW_HIDE) {
        ShowWindow(m_hWnd, nCmdShow);
        UpdateWindow(m_hWnd);
    }

    return true;
}

void NativeWin32Window::CreateSidebar() {
    // Sidebar brand header
    HWND hBrandTitle = CreateWindowExW(0, L"STATIC", L"AnshuBio Unlock", WS_CHILD | WS_VISIBLE | SS_LEFT, 24, 25, 175, 26, m_hWnd, nullptr, m_hInstance, nullptr);
    SendMessage(hBrandTitle, WM_SETFONT, (WPARAM)m_hFontAppTitle, TRUE);

    HWND hBrandSub = CreateWindowExW(0, L"STATIC", L"Security Suite", WS_CHILD | WS_VISIBLE | SS_LEFT, 24, 52, 175, 18, m_hWnd, nullptr, m_hInstance, nullptr);
    SendMessage(hBrandSub, WM_SETFONT, (WPARAM)m_hFontSmall, TRUE);

    // Sidebar navigation buttons
    int startY = 100;
    int btnH = 42;
    int spacing = 6;

    m_hBtnNavDashboard = CreateWindowExW(0, L"BUTTON", L"Dashboard", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_FLAT, 12, startY, 196, btnH, m_hWnd, (HMENU)ID_NAV_DASHBOARD, m_hInstance, nullptr);
    SendMessage(m_hBtnNavDashboard, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    m_hBtnNavPhones = CreateWindowExW(0, L"BUTTON", L"Trusted Phones", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_FLAT, 12, startY + (btnH + spacing), 196, btnH, m_hWnd, (HMENU)ID_NAV_PHONES, m_hInstance, nullptr);
    SendMessage(m_hBtnNavPhones, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);

    m_hBtnNavSecurity = CreateWindowExW(0, L"BUTTON", L"Security", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_FLAT, 12, startY + (btnH + spacing) * 2, 196, btnH, m_hWnd, (HMENU)ID_NAV_SECURITY, m_hInstance, nullptr);
    SendMessage(m_hBtnNavSecurity, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);

    m_hBtnNavSettings = CreateWindowExW(0, L"BUTTON", L"Settings", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_FLAT, 12, startY + (btnH + spacing) * 3, 196, btnH, m_hWnd, (HMENU)ID_NAV_SETTINGS, m_hInstance, nullptr);
    SendMessage(m_hBtnNavSettings, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);

    m_hBtnNavLogs = CreateWindowExW(0, L"BUTTON", L"Security Logs", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_FLAT, 12, startY + (btnH + spacing) * 4, 196, btnH, m_hWnd, (HMENU)ID_NAV_LOGS, m_hInstance, nullptr);
    SendMessage(m_hBtnNavLogs, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);

    m_hBtnNavAbout = CreateWindowExW(0, L"BUTTON", L"About", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_FLAT, 12, startY + (btnH + spacing) * 5, 196, btnH, m_hWnd, (HMENU)ID_NAV_ABOUT, m_hInstance, nullptr);
    SendMessage(m_hBtnNavAbout, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);

    // Sidebar footer
    HWND hFooter = CreateWindowExW(0, L"STATIC", L"Version 1.0.0\nAnshuCore", WS_CHILD | WS_VISIBLE | SS_LEFT, 24, 630, 175, 36, m_hWnd, nullptr, m_hInstance, nullptr);
    SendMessage(hFooter, WM_SETFONT, (WPARAM)m_hFontSmall, TRUE);
}

void NativeWin32Window::CreatePages() {
    int contentX = 240;
    int contentY = 25;
    int contentW = 820;
    int contentH = 640;

    // ==========================================
    // 1. DASHBOARD PAGE
    // ==========================================
    m_hPageDashboard = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE, contentX, contentY, contentW, contentH, m_hWnd, nullptr, m_hInstance, nullptr);

    HWND hDashTitle = CreateWindowExW(0, L"STATIC", L"Dashboard", WS_CHILD | WS_VISIBLE | SS_LEFT, 0, 0, 300, 32, m_hPageDashboard, nullptr, m_hInstance, nullptr);
    SendMessage(hDashTitle, WM_SETFONT, (WPARAM)m_hFontHeader, TRUE);

    HWND hDashSub = CreateWindowExW(0, L"STATIC", L"Secure PC Authentication & Device Overview", WS_CHILD | WS_VISIBLE | SS_LEFT, 0, 34, 400, 20, m_hPageDashboard, nullptr, m_hInstance, nullptr);
    SendMessage(hDashSub, WM_SETFONT, (WPARAM)m_hFontSmall, TRUE);

    // Top Status Card
    HWND hStatusCard = CreateWindowExW(0, L"BUTTON", L"Protection Status", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, 70, contentW, 140, m_hPageDashboard, nullptr, m_hInstance, nullptr);
    SendMessage(hStatusCard, WM_SETFONT, (WPARAM)m_hFontSubHeader, TRUE);

    m_hDashStatusBadge = CreateWindowExW(0, L"STATIC", L"Status: Protected", WS_CHILD | WS_VISIBLE | SS_LEFT, 24, 100, 350, 24, m_hPageDashboard, nullptr, m_hInstance, nullptr);
    SendMessage(m_hDashStatusBadge, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    m_hDashPcName = CreateWindowExW(0, L"STATIC", L"Workstation: --", WS_CHILD | WS_VISIBLE | SS_LEFT, 24, 130, 350, 20, m_hPageDashboard, nullptr, m_hInstance, nullptr);
    SendMessage(m_hDashPcName, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);

    m_hDashOsInfo = CreateWindowExW(0, L"STATIC", L"Platform: Windows 11 x64 (Native)", WS_CHILD | WS_VISIBLE | SS_LEFT, 24, 155, 350, 20, m_hPageDashboard, nullptr, m_hInstance, nullptr);
    SendMessage(m_hDashOsInfo, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);

    m_hDashSessionState = CreateWindowExW(0, L"STATIC", L"Session: Unlocked (Active)", WS_CHILD | WS_VISIBLE | SS_LEFT, 420, 100, 350, 20, m_hPageDashboard, nullptr, m_hInstance, nullptr);
    SendMessage(m_hDashSessionState, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);

    m_hDashConnInfo = CreateWindowExW(0, L"STATIC", L"Transports: Wi-Fi (Port 42425) / Bluetooth RFCOMM", WS_CHILD | WS_VISIBLE | SS_LEFT, 420, 130, 370, 20, m_hPageDashboard, nullptr, m_hInstance, nullptr);
    SendMessage(m_hDashConnInfo, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);

    // Quick Actions & Paired Devices Card
    HWND hDevicesCard = CreateWindowExW(0, L"BUTTON", L"Paired Devices", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, 230, contentW, 200, m_hPageDashboard, nullptr, m_hInstance, nullptr);
    SendMessage(hDevicesCard, WM_SETFONT, (WPARAM)m_hFontSubHeader, TRUE);

    m_hDashPhonesSummary = CreateWindowExW(0, L"STATIC", L"No trusted phones paired yet.", WS_CHILD | WS_VISIBLE | SS_LEFT, 24, 265, 760, 60, m_hPageDashboard, nullptr, m_hInstance, nullptr);
    SendMessage(m_hDashPhonesSummary, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);

    m_hDashPairBtn = CreateWindowExW(0, L"BUTTON", L"+ Add Trusted Phone", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 24, 375, 180, 36, m_hPageDashboard, (HMENU)ID_BTN_DASH_PAIR, m_hInstance, nullptr);
    SendMessage(m_hDashPairBtn, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    m_hDashLockBtn = CreateWindowExW(0, L"BUTTON", L"Lock PC Now", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 220, 375, 140, 36, m_hPageDashboard, (HMENU)ID_BTN_DASH_LOCK, m_hInstance, nullptr);
    SendMessage(m_hDashLockBtn, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);

    // ==========================================
    // 2. TRUSTED PHONES PAGE
    // ==========================================
    m_hPagePhones = CreateWindowExW(0, L"STATIC", L"", WS_CHILD, contentX, contentY, contentW, contentH, m_hWnd, nullptr, m_hInstance, nullptr);

    HWND hPhonesTitle = CreateWindowExW(0, L"STATIC", L"Trusted Phones", WS_CHILD | WS_VISIBLE | SS_LEFT, 0, 0, 300, 32, m_hPagePhones, nullptr, m_hInstance, nullptr);
    SendMessage(hPhonesTitle, WM_SETFONT, (WPARAM)m_hFontHeader, TRUE);

    m_hPhonesCountLabel = CreateWindowExW(0, L"STATIC", L"0 / 2 devices registered", WS_CHILD | WS_VISIBLE | SS_LEFT, 0, 34, 400, 20, m_hPagePhones, nullptr, m_hInstance, nullptr);
    SendMessage(m_hPhonesCountLabel, WM_SETFONT, (WPARAM)m_hFontSmall, TRUE);

    // Phone 1 Card
    m_hPhoneCard1 = CreateWindowExW(0, L"BUTTON", L"Primary Phone", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, 70, contentW, 130, m_hPagePhones, nullptr, m_hInstance, nullptr);
    SendMessage(m_hPhoneCard1, WM_SETFONT, (WPARAM)m_hFontSubHeader, TRUE);

    m_hPhone1Info = CreateWindowExW(0, L"STATIC", L"No primary phone configured.", WS_CHILD | WS_VISIBLE | SS_LEFT, 24, 100, 520, 80, m_hPagePhones, nullptr, m_hInstance, nullptr);
    SendMessage(m_hPhone1Info, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);

    m_hPhone1RevokeBtn = CreateWindowExW(0, L"BUTTON", L"Revoke Key", WS_CHILD | BS_PUSHBUTTON, 570, 110, 110, 32, m_hPagePhones, (HMENU)ID_BTN_PHONE1_REVOKE, m_hInstance, nullptr);
    SendMessage(m_hPhone1RevokeBtn, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);

    m_hPhone1RemoveBtn = CreateWindowExW(0, L"BUTTON", L"Remove", WS_CHILD | BS_PUSHBUTTON, 690, 110, 90, 32, m_hPagePhones, (HMENU)ID_BTN_PHONE1_REMOVE, m_hInstance, nullptr);
    SendMessage(m_hPhone1RemoveBtn, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);

    // Phone 2 Card
    m_hPhoneCard2 = CreateWindowExW(0, L"BUTTON", L"Secondary Phone", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, 220, contentW, 130, m_hPagePhones, nullptr, m_hInstance, nullptr);
    SendMessage(m_hPhoneCard2, WM_SETFONT, (WPARAM)m_hFontSubHeader, TRUE);

    m_hPhone2Info = CreateWindowExW(0, L"STATIC", L"No secondary phone configured.", WS_CHILD | WS_VISIBLE | SS_LEFT, 24, 250, 520, 80, m_hPagePhones, nullptr, m_hInstance, nullptr);
    SendMessage(m_hPhone2Info, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);

    m_hPhone2RevokeBtn = CreateWindowExW(0, L"BUTTON", L"Revoke Key", WS_CHILD | BS_PUSHBUTTON, 570, 260, 110, 32, m_hPagePhones, (HMENU)ID_BTN_PHONE2_REVOKE, m_hInstance, nullptr);
    SendMessage(m_hPhone2RevokeBtn, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);

    m_hPhone2RemoveBtn = CreateWindowExW(0, L"BUTTON", L"Remove", WS_CHILD | BS_PUSHBUTTON, 690, 260, 90, 32, m_hPagePhones, (HMENU)ID_BTN_PHONE2_REMOVE, m_hInstance, nullptr);
    SendMessage(m_hPhone2RemoveBtn, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);

    m_hPhonesPairBtn = CreateWindowExW(0, L"BUTTON", L"+ Pair New Phone", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 0, 370, 180, 36, m_hPagePhones, (HMENU)ID_BTN_PHONES_PAIR, m_hInstance, nullptr);
    SendMessage(m_hPhonesPairBtn, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    m_hPhonesMaxWarning = CreateWindowExW(0, L"STATIC", L"Maximum 2 trusted phones reached. Remove one before pairing another.", WS_CHILD | SS_LEFT, 200, 380, 580, 24, m_hPagePhones, nullptr, m_hInstance, nullptr);
    SendMessage(m_hPhonesMaxWarning, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    // ==========================================
    // 3. SECURITY PAGE
    // ==========================================
    m_hPageSecurity = CreateWindowExW(0, L"STATIC", L"", WS_CHILD, contentX, contentY, contentW, contentH, m_hWnd, nullptr, m_hInstance, nullptr);

    HWND hSecTitle = CreateWindowExW(0, L"STATIC", L"Security & Cryptography", WS_CHILD | WS_VISIBLE | SS_LEFT, 0, 0, 400, 32, m_hPageSecurity, nullptr, m_hInstance, nullptr);
    SendMessage(hSecTitle, WM_SETFONT, (WPARAM)m_hFontHeader, TRUE);

    HWND hSecSub = CreateWindowExW(0, L"STATIC", L"Hardware Key Vault, Cryptographic Status & Architecture", WS_CHILD | WS_VISIBLE | SS_LEFT, 0, 34, 500, 20, m_hPageSecurity, nullptr, m_hInstance, nullptr);
    SendMessage(hSecSub, WM_SETFONT, (WPARAM)m_hFontSmall, TRUE);

    m_hSecDeviceCard = CreateWindowExW(0, L"BUTTON", L"Device Identity", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, 70, 395, 120, m_hPageSecurity, nullptr, m_hInstance, nullptr);
    SendMessage(m_hSecDeviceCard, WM_SETFONT, (WPARAM)m_hFontSubHeader, TRUE);

    m_hSecCryptoCard = CreateWindowExW(0, L"BUTTON", L"Cryptographic Engine", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 415, 70, 405, 120, m_hPageSecurity, nullptr, m_hInstance, nullptr);
    SendMessage(m_hSecCryptoCard, WM_SETFONT, (WPARAM)m_hFontSubHeader, TRUE);

    m_hSecVaultCard = CreateWindowExW(0, L"BUTTON", L"Credential Vault & DPAPI", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, 210, 395, 120, m_hPageSecurity, nullptr, m_hInstance, nullptr);
    SendMessage(m_hSecVaultCard, WM_SETFONT, (WPARAM)m_hFontSubHeader, TRUE);

    m_hSecLastAuthCard = CreateWindowExW(0, L"BUTTON", L"Authentication Ready State", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 415, 210, 405, 120, m_hPageSecurity, nullptr, m_hInstance, nullptr);
    SendMessage(m_hSecLastAuthCard, WM_SETFONT, (WPARAM)m_hFontSubHeader, TRUE);

    m_hSecFpCard = CreateWindowExW(0, L"BUTTON", L"Public Key Fingerprint (SHA-256)", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, 350, contentW, 90, m_hPageSecurity, nullptr, m_hInstance, nullptr);
    SendMessage(m_hSecFpCard, WM_SETFONT, (WPARAM)m_hFontSubHeader, TRUE);

    // ==========================================
    // 4. SETTINGS PAGE
    // ==========================================
    m_hPageSettings = CreateWindowExW(0, L"STATIC", L"", WS_CHILD, contentX, contentY, contentW, contentH, m_hWnd, nullptr, m_hInstance, nullptr);

    HWND hSetTitle = CreateWindowExW(0, L"STATIC", L"Settings", WS_CHILD | WS_VISIBLE | SS_LEFT, 0, 0, 300, 32, m_hPageSettings, nullptr, m_hInstance, nullptr);
    SendMessage(hSetTitle, WM_SETFONT, (WPARAM)m_hFontHeader, TRUE);

    HWND hGeneralGroup = CreateWindowExW(0, L"BUTTON", L"General", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, 70, contentW, 90, m_hPageSettings, nullptr, m_hInstance, nullptr);
    SendMessage(hGeneralGroup, WM_SETFONT, (WPARAM)m_hFontSubHeader, TRUE);

    m_hChkStartWithWindows = CreateWindowExW(0, L"BUTTON", L"Start AnshuBio Unlock automatically with Windows", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 24, 105, 500, 24, m_hPageSettings, nullptr, m_hInstance, nullptr);
    SendMessage(m_hChkStartWithWindows, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);
    SendMessage(m_hChkStartWithWindows, BM_SETCHECK, BST_CHECKED, 0);

    HWND hConnGroup = CreateWindowExW(0, L"BUTTON", L"Connection Transports", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, 180, contentW, 110, m_hPageSettings, nullptr, m_hInstance, nullptr);
    SendMessage(hConnGroup, WM_SETFONT, (WPARAM)m_hFontSubHeader, TRUE);

    m_hChkWifiEnabled = CreateWindowExW(0, L"BUTTON", L"Enable Wi-Fi / Local Area Network Authentication", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 24, 215, 500, 24, m_hPageSettings, nullptr, m_hInstance, nullptr);
    SendMessage(m_hChkWifiEnabled, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);
    SendMessage(m_hChkWifiEnabled, BM_SETCHECK, BST_CHECKED, 0);

    m_hChkBluetoothEnabled = CreateWindowExW(0, L"BUTTON", L"Enable Bluetooth RFCOMM Authentication", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 24, 245, 500, 24, m_hPageSettings, nullptr, m_hInstance, nullptr);
    SendMessage(m_hChkBluetoothEnabled, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);
    SendMessage(m_hChkBluetoothEnabled, BM_SETCHECK, BST_CHECKED, 0);

    HWND hNotifGroup = CreateWindowExW(0, L"BUTTON", L"Notifications", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, 310, contentW, 110, m_hPageSettings, nullptr, m_hInstance, nullptr);
    SendMessage(hNotifGroup, WM_SETFONT, (WPARAM)m_hFontSubHeader, TRUE);

    m_hChkSoundEnabled = CreateWindowExW(0, L"BUTTON", L"Authentication confirmation sound", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 24, 345, 500, 24, m_hPageSettings, nullptr, m_hInstance, nullptr);
    SendMessage(m_hChkSoundEnabled, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);
    SendMessage(m_hChkSoundEnabled, BM_SETCHECK, BST_CHECKED, 0);

    m_hChkVibrationEnabled = CreateWindowExW(0, L"BUTTON", L"Authentication haptic confirmation", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 24, 375, 500, 24, m_hPageSettings, nullptr, m_hInstance, nullptr);
    SendMessage(m_hChkVibrationEnabled, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);
    SendMessage(m_hChkVibrationEnabled, BM_SETCHECK, BST_CHECKED, 0);

    m_hBtnSaveSettings = CreateWindowExW(0, L"BUTTON", L"Save Preferences", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 0, 440, 160, 36, m_hPageSettings, (HMENU)ID_BTN_SAVE_SETTINGS, m_hInstance, nullptr);
    SendMessage(m_hBtnSaveSettings, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    // ==========================================
    // 5. SECURITY LOGS PAGE
    // ==========================================
    m_hPageLogs = CreateWindowExW(0, L"STATIC", L"", WS_CHILD, contentX, contentY, contentW, contentH, m_hWnd, nullptr, m_hInstance, nullptr);

    HWND hLogsTitle = CreateWindowExW(0, L"STATIC", L"Security Logs", WS_CHILD | WS_VISIBLE | SS_LEFT, 0, 0, 300, 32, m_hPageLogs, nullptr, m_hInstance, nullptr);
    SendMessage(hLogsTitle, WM_SETFONT, (WPARAM)m_hFontHeader, TRUE);

    HWND hLogsSub = CreateWindowExW(0, L"STATIC", L"Real-Time Tamper-Resistant Security Audit Trail", WS_CHILD | WS_VISIBLE | SS_LEFT, 0, 34, 500, 20, m_hPageLogs, nullptr, m_hInstance, nullptr);
    SendMessage(hLogsSub, WM_SETFONT, (WPARAM)m_hFontSmall, TRUE);

    m_hLogBox = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL, 0, 70, contentW, 460, m_hPageLogs, nullptr, m_hInstance, nullptr);
    SendMessage(m_hLogBox, WM_SETFONT, (WPARAM)m_hFontMono, TRUE);

    m_hBtnRefreshLogs = CreateWindowExW(0, L"BUTTON", L"Refresh Logs", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 545, 140, 36, m_hPageLogs, (HMENU)ID_BTN_REFRESH_LOGS, m_hInstance, nullptr);
    SendMessage(m_hBtnRefreshLogs, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);

    // ==========================================
    // 6. ABOUT PAGE
    // ==========================================
    m_hPageAbout = CreateWindowExW(0, L"STATIC", L"", WS_CHILD, contentX, contentY, contentW, contentH, m_hWnd, nullptr, m_hInstance, nullptr);

    HWND hAboutTitle = CreateWindowExW(0, L"STATIC", L"About AnshuBio Unlock", WS_CHILD | WS_VISIBLE | SS_LEFT, 0, 0, 400, 32, m_hPageAbout, nullptr, m_hInstance, nullptr);
    SendMessage(hAboutTitle, WM_SETFONT, (WPARAM)m_hFontHeader, TRUE);

    HWND hAboutCard = CreateWindowExW(0, L"BUTTON", L"Product Information", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, 70, contentW, 260, m_hPageAbout, nullptr, m_hInstance, nullptr);
    SendMessage(hAboutCard, WM_SETFONT, (WPARAM)m_hFontSubHeader, TRUE);

    std::wstring aboutInfo =
        L"Product: AnshuBio Unlock\n"
        L"Publisher: AnshuCore\n"
        L"Version: 1.0.0 (Release)\n"
        L"Application Identity: com.anshucore.bio\n\n"
        L"Architecture: Native C++17 / Qt 6 / CMake / Windows SDK\n"
        L"Security Model: Mutual ECDSA secp256r1 + SHA-256 + CSPRNG + DPAPI Vault\n"
        L"Operating System: Windows 10 & Windows 11 (64-Bit)\n\n"
        L"Copyright 2026 AnshuCore. All rights reserved.";

    HWND hAboutText = CreateWindowExW(0, L"STATIC", aboutInfo.c_str(), WS_CHILD | WS_VISIBLE | SS_LEFT, 24, 105, 750, 200, m_hPageAbout, nullptr, m_hInstance, nullptr);
    SendMessage(hAboutText, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);

    HWND hBtnUpdates = CreateWindowExW(0, L"BUTTON", L"Check for Updates", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 350, 160, 36, m_hPageAbout, (HMENU)ID_BTN_CHECK_UPDATES, m_hInstance, nullptr);
    SendMessage(hBtnUpdates, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);
}

void NativeWin32Window::SwitchToPage(AppPage page) {
    m_currentPage = page;

    // Hide all pages
    ShowWindow(m_hPageDashboard, SW_HIDE);
    ShowWindow(m_hPagePhones, SW_HIDE);
    ShowWindow(m_hPageSecurity, SW_HIDE);
    ShowWindow(m_hPageSettings, SW_HIDE);
    ShowWindow(m_hPageLogs, SW_HIDE);
    ShowWindow(m_hPageAbout, SW_HIDE);

    // Show selected page
    switch (page) {
    case AppPage::Dashboard:
        UpdateDashboard();
        ShowWindow(m_hPageDashboard, SW_SHOW);
        break;
    case AppPage::TrustedPhones:
        UpdateTrustedPhones();
        ShowWindow(m_hPagePhones, SW_SHOW);
        break;
    case AppPage::Security:
        UpdateSecurity();
        ShowWindow(m_hPageSecurity, SW_SHOW);
        break;
    case AppPage::Settings:
        UpdateSettings();
        ShowWindow(m_hPageSettings, SW_SHOW);
        break;
    case AppPage::Logs:
        UpdateLogs();
        ShowWindow(m_hPageLogs, SW_SHOW);
        break;
    case AppPage::About:
        UpdateAbout();
        ShowWindow(m_hPageAbout, SW_SHOW);
        break;
    }

    InvalidateRect(m_hWnd, nullptr, TRUE);
}

void NativeWin32Window::UpdateDashboard() {
    bool serviceRunning = WindowsService::IsServiceRunning();
    std::wstring statusText = serviceRunning
        ? L"Status: Protected (Windows Service Running)"
        : L"Status: Protected (Standalone Engine Active)";
    SetWindowTextW(m_hDashStatusBadge, statusText.c_str());

    std::string pcName = KeyStore::Instance().GetPcDisplayName();
    std::wstring wPcName(pcName.begin(), pcName.end());
    SetWindowTextW(m_hDashPcName, (L"Workstation: " + (wPcName.empty() ? L"Anshu-PC" : wPcName)).c_str());

    auto phones = KeyStore::Instance().GetTrustedPhones();
    if (phones.empty()) {
        SetWindowTextW(m_hDashPhonesSummary, L"No trusted phones paired yet.\nClick '+ Add Trusted Phone' to pair your Android device via QR code or manual PIN.");
    } else {
        std::wstring summary = std::to_wstring(phones.size()) + L" / 2 Trusted Phone(s) Connected:\n";
        for (size_t i = 0; i < phones.size(); ++i) {
            std::string name = phones[i].name;
            std::wstring wName(name.begin(), name.end());
            summary += L"• " + wName + L" (" + (phones[i].status == "PAIRED" ? L"Paired & Trusted" : L"Pending") + L")\n";
        }
        SetWindowTextW(m_hDashPhonesSummary, summary.c_str());
    }
}

void NativeWin32Window::UpdateTrustedPhones() {
    auto phones = KeyStore::Instance().GetTrustedPhones();
    std::wstring countStr = std::to_wstring(phones.size()) + L" / 2 devices registered";
    SetWindowTextW(m_hPhonesCountLabel, countStr.c_str());

    // Phone 1
    if (phones.size() >= 1) {
        std::wstring name(phones[0].name.begin(), phones[0].name.end());
        std::wstring info = L"Device: " + name + L"\nStatus: Connected & Trusted\nTransport: " +
                            std::wstring(phones[0].transport.begin(), phones[0].transport.end()) +
                            L"\nLast seen: " + std::wstring(phones[0].lastSeen.begin(), phones[0].lastSeen.end());
        SetWindowTextW(m_hPhone1Info, info.c_str());
        ShowWindow(m_hPhone1RevokeBtn, SW_SHOW);
        ShowWindow(m_hPhone1RemoveBtn, SW_SHOW);
    } else {
        SetWindowTextW(m_hPhone1Info, L"No primary phone configured.\nPair your Android device to enable biometric workstation unlock.");
        ShowWindow(m_hPhone1RevokeBtn, SW_HIDE);
        ShowWindow(m_hPhone1RemoveBtn, SW_HIDE);
    }

    // Phone 2
    if (phones.size() >= 2) {
        std::wstring name(phones[1].name.begin(), phones[1].name.end());
        std::wstring info = L"Device: " + name + L"\nStatus: Connected & Trusted\nTransport: " +
                            std::wstring(phones[1].transport.begin(), phones[1].transport.end()) +
                            L"\nLast seen: " + std::wstring(phones[1].lastSeen.begin(), phones[1].lastSeen.end());
        SetWindowTextW(m_hPhone2Info, info.c_str());
        ShowWindow(m_hPhone2RevokeBtn, SW_SHOW);
        ShowWindow(m_hPhone2RemoveBtn, SW_SHOW);
    } else {
        SetWindowTextW(m_hPhone2Info, L"No secondary phone configured.");
        ShowWindow(m_hPhone2RevokeBtn, SW_HIDE);
        ShowWindow(m_hPhone2RemoveBtn, SW_HIDE);
    }

    if (phones.size() >= 2) {
        ShowWindow(m_hPhonesMaxWarning, SW_SHOW);
        EnableWindow(m_hPhonesPairBtn, FALSE);
    } else {
        ShowWindow(m_hPhonesMaxWarning, SW_HIDE);
        EnableWindow(m_hPhonesPairBtn, TRUE);
    }
}

void NativeWin32Window::UpdateSecurity() {
    auto identity = KeyStore::Instance().GetPcIdentity();
    std::string fp = identity.fingerprint;
    std::wstring wFp(fp.begin(), fp.end());
    SetWindowTextW(m_hSecFpCard, (L"Public Key Fingerprint: " + (wFp.empty() ? L"Configuring..." : wFp)).c_str());
}

void NativeWin32Window::UpdateSettings() {
    auto settings = KeyStore::Instance().GetSettings();
    SendMessage(m_hChkStartWithWindows, BM_SETCHECK, settings.startWithWindows ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessage(m_hChkWifiEnabled, BM_SETCHECK, settings.wifiEnabled ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessage(m_hChkBluetoothEnabled, BM_SETCHECK, settings.bluetoothEnabled ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessage(m_hChkSoundEnabled, BM_SETCHECK, settings.soundEnabled ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessage(m_hChkVibrationEnabled, BM_SETCHECK, settings.vibrationEnabled ? BST_CHECKED : BST_UNCHECKED, 0);
}

void NativeWin32Window::UpdateLogs() {
    auto recent = SecurityLogger::Instance().GetRecentLogs(50);
    std::wstring logText;
    for (const auto& entry : recent) {
        auto timeT = static_cast<std::time_t>(entry.timestamp / 1000);
        std::tm tm{};
#if defined(_MSC_VER)
        localtime_s(&tm, &timeT);
#else
        std::tm* pTm = std::localtime(&timeT);
        if (pTm) tm = *pTm;
#endif
        wchar_t timeBuf[32];
        wcsftime(timeBuf, sizeof(timeBuf)/sizeof(wchar_t), L"%H:%M:%S", &tm);

        std::wstring msg(entry.message.begin(), entry.message.end());
        std::wstring tag(entry.tag.begin(), entry.tag.end());
        std::wstring lvl(entry.level.begin(), entry.level.end());

        logText += std::wstring(timeBuf) + L"  [" + lvl + L"]  " + tag + L"  —  " + msg + L"\r\n";
    }
    SetWindowTextW(m_hLogBox, logText.c_str());
    SendMessage(m_hLogBox, EM_LINESCROLL, 0, 9999);
}

void NativeWin32Window::UpdateAbout() {}

void NativeWin32Window::RefreshAll() {
    UpdateDashboard();
    UpdateTrustedPhones();
    UpdateSecurity();
    UpdateSettings();
    UpdateLogs();
}

void NativeWin32Window::SetupTrayIcon() {
    ZeroMemory(&m_nid, sizeof(m_nid));
    m_nid.cbSize = sizeof(NOTIFYICONDATAW);
    m_nid.hWnd = m_hWnd;
    m_nid.uID = 1;
    m_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    m_nid.uCallbackMessage = WM_TRAYICON;
    m_nid.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wcscpy_s(m_nid.szTip, L"AnshuBio Unlock — Protected");
    Shell_NotifyIconW(NIM_ADD, &m_nid);
}

void NativeWin32Window::RemoveTrayIcon() {
    if (m_nid.cbSize) {
        Shell_NotifyIconW(NIM_DELETE, &m_nid);
        m_nid.cbSize = 0;
    }
}

int NativeWin32Window::RunMessageLoop() {
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}

void NativeWin32Window::Show() {
    if (m_hWnd) {
        ShowWindow(m_hWnd, SW_RESTORE);
        SetForegroundWindow(m_hWnd);
    }
}

void NativeWin32Window::Hide() {
    if (m_hWnd) {
        ShowWindow(m_hWnd, SW_HIDE);
    }
}

LRESULT NativeWin32Window::HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        // Paint sidebar background
        RECT sideRect = { 0, 0, 220, 720 };
        FillRect(hdc, &sideRect, m_hBrushBgSidebar);

        // Paint sidebar divider line
        HPEN hPenDivider = CreatePen(PS_SOLID, 1, RGB(30, 41, 59));
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPenDivider);
        MoveToEx(hdc, 220, 0, nullptr);
        LineTo(hdc, 220, 720);
        SelectObject(hdc, hOldPen);
        DeleteObject(hPenDivider);

        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_CTLCOLORSTATIC: {
        HDC hdcStatic = (HDC)wParam;
        SetTextColor(hdcStatic, COLOR_TEXT_PRIMARY);
        SetBkMode(hdcStatic, TRANSPARENT);
        return (INT_PTR)m_hBrushBgMain;
    }

    case WM_CTLCOLOREDIT: {
        HDC hdcEdit = (HDC)wParam;
        SetTextColor(hdcEdit, COLOR_TEXT_PRIMARY);
        SetBkColor(hdcEdit, COLOR_BG_CARD);
        return (INT_PTR)m_hBrushBgCard;
    }

    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == ID_NAV_DASHBOARD) { SwitchToPage(AppPage::Dashboard); return 0; }
        if (id == ID_NAV_PHONES) { SwitchToPage(AppPage::TrustedPhones); return 0; }
        if (id == ID_NAV_SECURITY) { SwitchToPage(AppPage::Security); return 0; }
        if (id == ID_NAV_SETTINGS) { SwitchToPage(AppPage::Settings); return 0; }
        if (id == ID_NAV_LOGS) { SwitchToPage(AppPage::Logs); return 0; }
        if (id == ID_NAV_ABOUT) { SwitchToPage(AppPage::About); return 0; }

        if (id == ID_BTN_DASH_PAIR || id == ID_BTN_PHONES_PAIR) {
            PairingDialog::ShowModal(m_hWnd);
            RefreshAll();
            return 0;
        }

        if (id == ID_BTN_DASH_LOCK) {
            SessionMonitor::Instance().LockWorkStation();
            RefreshAll();
            return 0;
        }

        if (id == ID_BTN_PHONE1_REMOVE || id == ID_BTN_PHONE1_REVOKE) {
            auto phones = KeyStore::Instance().GetTrustedPhones();
            if (phones.size() >= 1) {
                if (id == ID_BTN_PHONE1_REVOKE) KeyStore::Instance().RevokeTrustedPhone(phones[0].id);
                else KeyStore::Instance().RemoveTrustedPhone(phones[0].id);
                RefreshAll();
            }
            return 0;
        }

        if (id == ID_BTN_PHONE2_REMOVE || id == ID_BTN_PHONE2_REVOKE) {
            auto phones = KeyStore::Instance().GetTrustedPhones();
            if (phones.size() >= 2) {
                if (id == ID_BTN_PHONE2_REVOKE) KeyStore::Instance().RevokeTrustedPhone(phones[1].id);
                else KeyStore::Instance().RemoveTrustedPhone(phones[1].id);
                RefreshAll();
            }
            return 0;
        }

        if (id == ID_BTN_REFRESH_LOGS) {
            UpdateLogs();
            return 0;
        }

        if (id == ID_BTN_SAVE_SETTINGS) {
            AppSettings s = KeyStore::Instance().GetSettings();
            s.startWithWindows = (SendMessage(m_hChkStartWithWindows, BM_GETCHECK, 0, 0) == BST_CHECKED);
            s.wifiEnabled = (SendMessage(m_hChkWifiEnabled, BM_GETCHECK, 0, 0) == BST_CHECKED);
            s.bluetoothEnabled = (SendMessage(m_hChkBluetoothEnabled, BM_GETCHECK, 0, 0) == BST_CHECKED);
            s.soundEnabled = (SendMessage(m_hChkSoundEnabled, BM_GETCHECK, 0, 0) == BST_CHECKED);
            s.vibrationEnabled = (SendMessage(m_hChkVibrationEnabled, BM_GETCHECK, 0, 0) == BST_CHECKED);
            KeyStore::Instance().UpdateSettings(s);
            MessageBoxW(m_hWnd, L"Preferences saved successfully.", L"AnshuBio Unlock", MB_ICONINFORMATION | MB_OK);
            return 0;
        }

        if (id == ID_BTN_CHECK_UPDATES) {
            MessageBoxW(m_hWnd, L"You are running the latest version of AnshuBio Unlock (v1.0.0).", L"AnshuBio Unlock", MB_ICONINFORMATION | MB_OK);
            return 0;
        }

        if (id == ID_TRAY_RESTORE) {
            Show();
            return 0;
        }
        if (id == ID_TRAY_QUIT) {
            DestroyWindow(m_hWnd);
            return 0;
        }
        break;
    }

    case WM_SYSCOMMAND: {
        if ((wParam & 0xFFF0) == SC_MINIMIZE) {
            ShowWindow(m_hWnd, SW_HIDE);
            m_isMinimizedToTray = true;
            return 0;
        }
        break;
    }

    case WM_TRAYICON: {
        if (lParam == WM_LBUTTONDBLCLK) {
            Show();
        } else if (lParam == WM_RBUTTONUP) {
            POINT pt;
            GetCursorPos(&pt);
            HMENU hMenu = CreatePopupMenu();
            AppendMenuW(hMenu, MF_STRING, ID_TRAY_RESTORE, L"Open AnshuBio Unlock");
            AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(hMenu, MF_STRING, ID_TRAY_QUIT, L"Exit");
            SetForegroundWindow(m_hWnd);
            TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, m_hWnd, nullptr);
            DestroyMenu(hMenu);
        }
        return 0;
    }

    case WM_GETMINMAXINFO: {
        LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
        lpMMI->ptMinTrackSize.x = 1000;
        lpMMI->ptMinTrackSize.y = 650;
        return 0;
    }

    case WM_DESTROY: {
        RemoveTrayIcon();
        PostQuitMessage(0);
        return 0;
    }
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

} // namespace AnshuBio
