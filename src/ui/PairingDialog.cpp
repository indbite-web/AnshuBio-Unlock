#include "PairingDialog.h"
#include "../core/Constants.h"
#include "../storage/KeyStore.h"
#include "../storage/SecurityLogger.h"
#include <sstream>

#define IDT_PAIRING_TIMER 3001
#define ID_TAB_PAIRING 3002
#define ID_BTN_CONFIRM_PAIR 3003
#define ID_BTN_CANCEL_PAIR 3004

namespace AnshuBio {

static PairingDialog* s_activeDialog = nullptr;

static LRESULT CALLBACK PairingDialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (s_activeDialog) {
        return s_activeDialog->HandleMessage(hWnd, msg, wParam, lParam);
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

bool PairingDialog::ShowModal(HWND hParent) {
    // 1. Enforce max 2 trusted phones rule
    if (KeyStore::Instance().GetTrustedPhones().size() >= 2) {
        MessageBoxW(
            hParent,
            L"Maximum of 2 trusted phones reached.\n\nPlease remove or revoke an existing phone from the Trusted Phones list before pairing a new one.",
            L"Limit Reached — AnshuBio Unlock",
            MB_ICONWARNING | MB_OK
        );
        return false;
    }

    PairingDialog dlg;
    s_activeDialog = &dlg;
    bool res = dlg.Create(hParent);
    s_activeDialog = nullptr;
    return res;
}

PairingDialog::PairingDialog() {}

PairingDialog::~PairingDialog() {
    if (m_hQrBitmap) {
        DeleteObject(m_hQrBitmap);
        m_hQrBitmap = nullptr;
    }
}

bool PairingDialog::Create(HWND hParent) {
    m_hParent = hParent;

    // 2. Initiate active pairing session
    m_session = AuthCoordinator::Instance().InitiatePairingSession();
    if (!m_session.has_value()) {
        return false;
    }

    m_remainingSeconds = 60;

    // 3. Generate QR Code using Nayuki's algorithm
    try {
        m_qrCode = std::make_unique<qrcodegen::QrCode>(
            qrcodegen::QrCode::encodeText(m_session->qrPayload.c_str(), qrcodegen::QrCode::Ecc::MEDIUM)
        );
    } catch (const std::exception& e) {
        SecurityLogger::Instance().Error("QR", std::string("Failed to generate QR code: ") + e.what());
    }

    WNDCLASSEXW wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = PairingDialogProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.lpszClassName = L"AnshuBioPairingDialogClass";

    RegisterClassExW(&wc);

    RECT parentRect;
    GetWindowRect(hParent, &parentRect);
    int width = 460;
    int height = 520;
    int x = parentRect.left + (parentRect.right - parentRect.left - width) / 2;
    int y = parentRect.top + (parentRect.bottom - parentRect.top - height) / 2;

    m_hWnd = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        wc.lpszClassName,
        L"Pair New Phone — AnshuBio Unlock",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        x, y,
        width, height,
        hParent,
        nullptr,
        wc.hInstance,
        nullptr
    );

    if (!m_hWnd) return false;

    CreateControls();
    SwitchMode(0); // Default to QR Code
    SetTimer(m_hWnd, IDT_PAIRING_TIMER, 1000, nullptr);

    EnableWindow(hParent, FALSE);

    MSG msg;
    while (IsWindow(m_hWnd) && GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    EnableWindow(hParent, TRUE);
    SetForegroundWindow(hParent);

    return true;
}

void PairingDialog::CreateControls() {
    HINSTANCE hInst = GetModuleHandleW(nullptr);

    HFONT hFontTitle = CreateFontW(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    HFONT hFontNormal = CreateFontW(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    HFONT hFontBold = CreateFontW(15, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    HFONT hFontCode = CreateFontW(32, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Consolas");

    // Header Title
    HWND hTitle = CreateWindowExW(0, L"STATIC", L"Pair New Phone", WS_CHILD | WS_VISIBLE | SS_CENTER, 20, 15, 400, 24, m_hWnd, nullptr, hInst, nullptr);
    SendMessage(hTitle, WM_SETFONT, (WPARAM)hFontTitle, TRUE);

    // Tab Control for QR Code vs Manual Code
    m_hTab = CreateWindowExW(0, WC_TABCONTROLW, L"", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, 20, 45, 405, 30, m_hWnd, (HMENU)ID_TAB_PAIRING, hInst, nullptr);
    SendMessage(m_hTab, WM_SETFONT, (WPARAM)hFontNormal, TRUE);

    TCITEMW tie;
    tie.mask = TCIF_TEXT;
    tie.pszText = const_cast<LPWSTR>(L"QR Code");
    TabCtrl_InsertItem(m_hTab, 0, &tie);
    tie.pszText = const_cast<LPWSTR>(L"Pairing Code");
    TabCtrl_InsertItem(m_hTab, 1, &tie);

    // Instructions
    m_hQrCodeText = CreateWindowExW(0, L"STATIC", L"Scan this QR code with AnshuBio Unlock on Android", WS_CHILD | WS_VISIBLE | SS_CENTER, 20, 85, 400, 20, m_hWnd, nullptr, hInst, nullptr);
    SendMessage(m_hQrCodeText, WM_SETFONT, (WPARAM)hFontNormal, TRUE);

    // Manual Code Large Display
    std::string codeStr = m_session ? m_session->confirmCode : "------";
    std::wstring wCode(codeStr.begin(), codeStr.end());
    m_hCodeLabel = CreateWindowExW(0, L"STATIC", wCode.c_str(), WS_CHILD | SS_CENTER, 20, 160, 400, 45, m_hWnd, nullptr, hInst, nullptr);
    SendMessage(m_hCodeLabel, WM_SETFONT, (WPARAM)hFontCode, TRUE);

    // PC Info details for manual pairing
    std::wstring pcInfo = L"PC Name: " + std::wstring(m_session->pcName.begin(), m_session->pcName.end()) +
                          L"\nIP Address: " + std::wstring(m_session->ipAddress.begin(), m_session->ipAddress.end()) +
                          L"\nPort: " + std::to_wstring(m_session->port);
    m_hPcInfoLabel = CreateWindowExW(0, L"STATIC", pcInfo.c_str(), WS_CHILD | SS_CENTER, 20, 230, 400, 60, m_hWnd, nullptr, hInst, nullptr);
    SendMessage(m_hPcInfoLabel, WM_SETFONT, (WPARAM)hFontNormal, TRUE);

    // Pairing PIN Subtitle under QR
    HWND hPinSub = CreateWindowExW(0, L"STATIC", (L"Pairing Code: " + wCode).c_str(), WS_CHILD | WS_VISIBLE | SS_CENTER, 20, 335, 400, 20, m_hWnd, nullptr, hInst, nullptr);
    SendMessage(hPinSub, WM_SETFONT, (WPARAM)hFontBold, TRUE);

    // Timer Label
    m_hTimerLabel = CreateWindowExW(0, L"STATIC", L"Expires in: 60 seconds", WS_CHILD | WS_VISIBLE | SS_CENTER, 20, 360, 400, 20, m_hWnd, nullptr, hInst, nullptr);
    SendMessage(m_hTimerLabel, WM_SETFONT, (WPARAM)hFontNormal, TRUE);

    // Status Label
    m_hStatusLabel = CreateWindowExW(0, L"STATIC", L"Waiting for phone...", WS_CHILD | WS_VISIBLE | SS_CENTER, 20, 385, 400, 22, m_hWnd, nullptr, hInst, nullptr);
    SendMessage(m_hStatusLabel, WM_SETFONT, (WPARAM)hFontBold, TRUE);

    // Mutual Confirmation Prompt & Buttons
    m_hConfirmPrompt = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | SS_CENTER, 20, 410, 400, 22, m_hWnd, nullptr, hInst, nullptr);
    SendMessage(m_hConfirmPrompt, WM_SETFONT, (WPARAM)hFontBold, TRUE);

    m_hConfirmBtn = CreateWindowExW(0, L"BUTTON", L"Confirm Pairing", WS_CHILD | BS_DEFPUSHBUTTON, 110, 435, 120, 30, m_hWnd, (HMENU)ID_BTN_CONFIRM_PAIR, hInst, nullptr);
    SendMessage(m_hConfirmBtn, WM_SETFONT, (WPARAM)hFontNormal, TRUE);

    m_hCancelBtn = CreateWindowExW(0, L"BUTTON", L"Cancel", WS_CHILD | BS_PUSHBUTTON, 240, 435, 100, 30, m_hWnd, (HMENU)ID_BTN_CANCEL_PAIR, hInst, nullptr);
    SendMessage(m_hCancelBtn, WM_SETFONT, (WPARAM)hFontNormal, TRUE);
}

void PairingDialog::SwitchMode(int mode) {
    m_currentMode = mode;
    if (mode == 0) { // QR Code
        ShowWindow(m_hQrCodeText, SW_SHOW);
        ShowWindow(m_hCodeLabel, SW_HIDE);
        ShowWindow(m_hPcInfoLabel, SW_HIDE);
    } else { // Manual Code
        ShowWindow(m_hQrCodeText, SW_HIDE);
        ShowWindow(m_hCodeLabel, SW_SHOW);
        ShowWindow(m_hPcInfoLabel, SW_SHOW);
    }
    InvalidateRect(m_hWnd, nullptr, TRUE);
}

void PairingDialog::UpdateTimerDisplay() {
    if (m_remainingSeconds > 0) {
        std::wstring text = L"Expires in: " + std::to_wstring(m_remainingSeconds) + L" seconds";
        SetWindowTextW(m_hTimerLabel, text.c_str());
    } else {
        SetWindowTextW(m_hTimerLabel, L"Pairing session expired");
        SetWindowTextW(m_hStatusLabel, L"Session Expired. Please retry.");
        KillTimer(m_hWnd, IDT_PAIRING_TIMER);
    }
}

void PairingDialog::DrawQrCode(HDC hdc, const RECT& rect) {
    if (!m_qrCode || m_currentMode != 0) return;

    int qrSize = m_qrCode->getSize();
    int border = 4;
    int totalModules = qrSize + (border * 2);

    int targetPixels = 200;
    int scale = targetPixels / totalModules;
    if (scale < 1) scale = 1;

    int actualSize = totalModules * scale;
    int startX = rect.left + ((rect.right - rect.left) - actualSize) / 2;
    int startY = 115;

    // Draw white background quiet zone
    HBRUSH hWhiteBrush = (HBRUSH)GetStockObject(WHITE_BRUSH);
    RECT bgRect = { startX, startY, startX + actualSize, startY + actualSize };
    FillRect(hdc, &bgRect, hWhiteBrush);

    // Draw dark modules
    HBRUSH hBlackBrush = (HBRUSH)GetStockObject(BLACK_BRUSH);
    for (int y = 0; y < qrSize; y++) {
        for (int x = 0; x < qrSize; x++) {
            if (m_qrCode->getModule(x, y)) {
                RECT modRect;
                modRect.left = startX + (x + border) * scale;
                modRect.top = startY + (y + border) * scale;
                modRect.right = modRect.left + scale;
                modRect.bottom = modRect.top + scale;
                FillRect(hdc, &modRect, hBlackBrush);
            }
        }
    }
}

void PairingDialog::CheckPairingStatus() {
    auto activeSession = AuthCoordinator::Instance().GetActivePairingSession();
    if (!activeSession.has_value()) {
        if (m_remainingSeconds <= 0) {
            SetWindowTextW(m_hStatusLabel, L"Pairing session expired.");
        }
        return;
    }

    if (activeSession->status == "CONFIRMATION_REQUIRED") {
        std::string phoneName = activeSession->pendingPhone.name;
        std::wstring wPhone(phoneName.begin(), phoneName.end());
        SetWindowTextW(m_hStatusLabel, (L"Phone Found: " + wPhone).c_str());
        SetWindowTextW(m_hConfirmPrompt, (L"Pair with " + wPhone + L"?").c_str());
        ShowWindow(m_hConfirmPrompt, SW_SHOW);
        ShowWindow(m_hConfirmBtn, SW_SHOW);
        SetWindowPos(m_hCancelBtn, nullptr, 240, 435, 100, 30, SWP_NOZORDER);
    } else if (activeSession->status == "PAIRED") {
        SetWindowTextW(m_hStatusLabel, L"Pairing successful! PC is now protected.");
        KillTimer(m_hWnd, IDT_PAIRING_TIMER);
        Sleep(1200);
        DestroyWindow(m_hWnd);
    }
}

LRESULT PairingDialog::HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RECT clientRect;
        GetClientRect(hWnd, &clientRect);
        DrawQrCode(hdc, clientRect);
        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_NOTIFY: {
        LPNMHDR pnmhdr = (LPNMHDR)lParam;
        if (pnmhdr->idFrom == ID_TAB_PAIRING && pnmhdr->code == TCN_SELCHANGE) {
            int curSel = TabCtrl_GetCurSel(m_hTab);
            SwitchMode(curSel);
            return 0;
        }
        break;
    }

    case WM_TIMER: {
        if (wParam == IDT_PAIRING_TIMER) {
            if (m_remainingSeconds > 0) {
                m_remainingSeconds--;
                UpdateTimerDisplay();
                CheckPairingStatus();
            }
            return 0;
        }
        break;
    }

    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == ID_BTN_CONFIRM_PAIR) {
            if (m_session.has_value()) {
                AuthCoordinator::Instance().ConfirmPairingFromPc(m_session->sessionId);
                SetWindowTextW(m_hStatusLabel, L"Pairing confirmed. Completing handshake...");
                CheckPairingStatus();
            }
            return 0;
        }
        if (id == ID_BTN_CANCEL_PAIR || id == IDCANCEL) {
            AuthCoordinator::Instance().CancelPairing();
            KillTimer(hWnd, IDT_PAIRING_TIMER);
            DestroyWindow(hWnd);
            return 0;
        }
        break;
    }

    case WM_CLOSE: {
        AuthCoordinator::Instance().CancelPairing();
        KillTimer(hWnd, IDT_PAIRING_TIMER);
        DestroyWindow(hWnd);
        return 0;
    }

    case WM_DESTROY: {
        KillTimer(hWnd, IDT_PAIRING_TIMER);
        return 0;
    }
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

} // namespace AnshuBio
