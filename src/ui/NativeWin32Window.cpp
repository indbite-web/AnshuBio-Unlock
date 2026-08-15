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

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")

#define WM_TRAYICON (WM_USER + 101)
#define ID_BTN_PAIR 1001
#define ID_BTN_LOCK 1002
#define ID_BTN_REFRESH 1003
#define ID_TRAY_RESTORE 2001
#define ID_TRAY_QUIT 2002

namespace AnshuBio {

static LRESULT CALLBACK MainWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    return NativeWin32Window::Instance().HandleMessage(hWnd, msg, wParam, lParam);
}

NativeWin32Window& NativeWin32Window::Instance() {
    static NativeWin32Window s_instance;
    return s_instance;
}

NativeWin32Window::NativeWin32Window() {}

NativeWin32Window::~NativeWin32Window() {
    RemoveTrayIcon();
}

bool NativeWin32Window::Initialize(HINSTANCE hInstance, int nCmdShow) {
    m_hInstance = hInstance;

    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_TAB_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);

    WNDCLASSEXW wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = MainWindowProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.lpszClassName = L"AnshuBioUnlockMainWindowClass";

    RegisterClassExW(&wc);

    m_hWnd = CreateWindowExW(
        WS_EX_APPWINDOW,
        wc.lpszClassName,
        L"AnshuBio Unlock",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        780, 560,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (!m_hWnd) {
        return false;
    }

    CreateControls();
    SetupTrayIcon();
    UpdateDashboard();
    UpdateLogs();

    if (nCmdShow != SW_HIDE) {
        ShowWindow(m_hWnd, nCmdShow);
        UpdateWindow(m_hWnd);
    }

    return true;
}

void NativeWin32Window::CreateControls() {
    HFONT hFontTitle = CreateFontW(22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    HFONT hFontNormal = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    HFONT hFontMono = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FIXED_PITCH | FF_DONTCARE, L"Consolas");

    // Title Header
    HWND hTitle = CreateWindowExW(0, L"STATIC", L"AnshuBio Unlock", WS_CHILD | WS_VISIBLE | SS_LEFT, 24, 20, 300, 30, m_hWnd, nullptr, m_hInstance, nullptr);
    SendMessage(hTitle, WM_SETFONT, (WPARAM)hFontTitle, TRUE);

    // Status Banner
    m_hStatusLabel = CreateWindowExW(0, L"STATIC", L"Status: Initializing...", WS_CHILD | WS_VISIBLE | SS_LEFT, 24, 55, 500, 22, m_hWnd, nullptr, m_hInstance, nullptr);
    SendMessage(m_hStatusLabel, WM_SETFONT, (WPARAM)hFontNormal, TRUE);

    // PC Name & Fingerprint Group
    HWND hGroupInfo = CreateWindowExW(0, L"BUTTON", L"Workstation Identity", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 24, 90, 715, 85, m_hWnd, nullptr, m_hInstance, nullptr);
    SendMessage(hGroupInfo, WM_SETFONT, (WPARAM)hFontNormal, TRUE);

    m_hPcNameLabel = CreateWindowExW(0, L"STATIC", L"PC Name: --", WS_CHILD | WS_VISIBLE | SS_LEFT, 40, 115, 300, 20, m_hWnd, nullptr, m_hInstance, nullptr);
    SendMessage(m_hPcNameLabel, WM_SETFONT, (WPARAM)hFontNormal, TRUE);

    m_hFingerprintLabel = CreateWindowExW(0, L"STATIC", L"Fingerprint: --", WS_CHILD | WS_VISIBLE | SS_LEFT, 40, 140, 450, 20, m_hWnd, nullptr, m_hInstance, nullptr);
    SendMessage(m_hFingerprintLabel, WM_SETFONT, (WPARAM)hFontNormal, TRUE);

    // Trusted Phones Group
    HWND hGroupPhones = CreateWindowExW(0, L"BUTTON", L"Paired Phones", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 24, 190, 715, 120, m_hWnd, nullptr, m_hInstance, nullptr);
    SendMessage(hGroupPhones, WM_SETFONT, (WPARAM)hFontNormal, TRUE);

    m_hPhoneList = CreateWindowExW(0, L"STATIC", L"No trusted phones paired yet.", WS_CHILD | WS_VISIBLE | SS_LEFT, 40, 215, 680, 50, m_hWnd, nullptr, m_hInstance, nullptr);
    SendMessage(m_hPhoneList, WM_SETFONT, (WPARAM)hFontNormal, TRUE);

    m_hPairBtn = CreateWindowExW(0, L"BUTTON", L"Pair New Phone", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 40, 270, 140, 30, m_hWnd, (HMENU)ID_BTN_PAIR, m_hInstance, nullptr);
    SendMessage(m_hPairBtn, WM_SETFONT, (WPARAM)hFontNormal, TRUE);

    m_hPairCodeLabel = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_LEFT, 200, 275, 400, 25, m_hWnd, nullptr, m_hInstance, nullptr);
    SendMessage(m_hPairCodeLabel, WM_SETFONT, (WPARAM)hFontNormal, TRUE);

    // Actions & Security Log
    HWND hGroupLogs = CreateWindowExW(0, L"BUTTON", L"Security Audit Log", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 24, 325, 715, 175, m_hWnd, nullptr, m_hInstance, nullptr);
    SendMessage(hGroupLogs, WM_SETFONT, (WPARAM)hFontNormal, TRUE);

    m_hLogBox = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL, 40, 350, 680, 110, m_hWnd, nullptr, m_hInstance, nullptr);
    SendMessage(m_hLogBox, WM_SETFONT, (WPARAM)hFontMono, TRUE);

    m_hLockBtn = CreateWindowExW(0, L"BUTTON", L"Lock PC Now", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 40, 465, 120, 26, m_hWnd, (HMENU)ID_BTN_LOCK, m_hInstance, nullptr);
    SendMessage(m_hLockBtn, WM_SETFONT, (WPARAM)hFontNormal, TRUE);

    HWND hRefreshBtn = CreateWindowExW(0, L"BUTTON", L"Refresh Logs", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 170, 465, 120, 26, m_hWnd, (HMENU)ID_BTN_REFRESH, m_hInstance, nullptr);
    SendMessage(hRefreshBtn, WM_SETFONT, (WPARAM)hFontNormal, TRUE);
}

void NativeWin32Window::UpdateDashboard() {
    bool serviceRunning = WindowsService::IsServiceRunning();
    std::wstring statusText = serviceRunning
        ? L"Status: ● Windows Service Active (Protected)"
        : L"Status: ● Local Standalone Engine Active";

    SetWindowTextW(m_hStatusLabel, statusText.c_str());

    std::string pcName = KeyStore::Instance().GetPcDisplayName();
    std::wstring wPcName(pcName.begin(), pcName.end());
    SetWindowTextW(m_hPcNameLabel, (L"PC Name: " + wPcName).c_str());

    std::string fp = KeyStore::Instance().GetPcIdentity().fingerprint;
    std::wstring wFp(fp.begin(), fp.end());
    SetWindowTextW(m_hFingerprintLabel, (L"SHA-256 Fingerprint: " + (wFp.empty() ? L"Configuring..." : wFp)).c_str());

    RefreshPhoneList();
}

void NativeWin32Window::RefreshPhoneList() {
    auto phones = KeyStore::Instance().GetTrustedPhones();
    if (phones.empty()) {
        SetWindowTextW(m_hPhoneList, L"No trusted phones paired yet. Click 'Pair New Phone' to begin mutual confirmation.");
    } else {
        std::wstring text = L"";
        for (size_t i = 0; i < phones.size(); ++i) {
            std::string name = phones[i].name;
            std::wstring wName(name.begin(), name.end());
            text += std::to_wstring(i + 1) + L". " + wName + L" (" + (phones[i].status == "PAIRED" ? L"Paired & Trusted" : L"Pending") + L")\n";
        }
        SetWindowTextW(m_hPhoneList, text.c_str());
    }
}

void NativeWin32Window::UpdateLogs() {
    auto recent = SecurityLogger::Instance().GetRecentLogs(30);
    std::wstring logText;
    for (const auto& entry : recent) {
        std::wstring msg(entry.message.begin(), entry.message.end());
        std::wstring tag(entry.tag.begin(), entry.tag.end());
        std::wstring lvl(entry.level.begin(), entry.level.end());
        logText += L"[" + lvl + L"] [" + tag + L"] " + msg + L"\r\n";
    }
    SetWindowTextW(m_hLogBox, logText.c_str());
    SendMessage(m_hLogBox, EM_LINESCROLL, 0, 9999);
}

void NativeWin32Window::SetupTrayIcon() {
    ZeroMemory(&m_nid, sizeof(m_nid));
    m_nid.cbSize = sizeof(NOTIFYICONDATAW);
    m_nid.hWnd = m_hWnd;
    m_nid.uID = 1;
    m_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    m_nid.uCallbackMessage = WM_TRAYICON;
    m_nid.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wcscpy_s(m_nid.szTip, L"AnshuBio Unlock");
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
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == ID_BTN_LOCK) {
            SessionMonitor::Instance().LockWorkStation();
            UpdateDashboard();
            UpdateLogs();
            return 0;
        }
        if (id == ID_BTN_REFRESH) {
            UpdateDashboard();
            UpdateLogs();
            return 0;
        }
        if (id == ID_BTN_PAIR) {
            PairingDialog::ShowModal(m_hWnd);
            UpdateDashboard();
            UpdateLogs();
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

    case WM_DESTROY: {
        RemoveTrayIcon();
        PostQuitMessage(0);
        return 0;
    }
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

} // namespace AnshuBio
