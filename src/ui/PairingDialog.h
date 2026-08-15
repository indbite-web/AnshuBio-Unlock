#pragma once
#include <windows.h>
#include <commctrl.h>
#include <string>
#include <memory>
#include "../core/AuthCoordinator.h"
#include "../crypto/QrCode.hpp"

namespace AnshuBio {

class PairingDialog {
public:
    static bool ShowModal(HWND hParent);

    LRESULT HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    PairingDialog();
    ~PairingDialog();

    bool Create(HWND hParent);
    void CreateControls();
    void SwitchMode(int mode); // 0 = QR Code, 1 = Manual Code
    void UpdateTimerDisplay();
    void DrawQrCode(HDC hdc, const RECT& rect);
    void CheckPairingStatus();

    HWND m_hWnd = nullptr;
    HWND m_hParent = nullptr;
    HWND m_hTab = nullptr;
    HWND m_hQrPanel = nullptr;
    HWND m_hCodePanel = nullptr;
    HWND m_hQrCodeText = nullptr;
    HWND m_hCodeLabel = nullptr;
    HWND m_hTimerLabel = nullptr;
    HWND m_hStatusLabel = nullptr;
    HWND m_hConfirmPrompt = nullptr;
    HWND m_hConfirmBtn = nullptr;
    HWND m_hCancelBtn = nullptr;
    HWND m_hPcInfoLabel = nullptr;

    int m_currentMode = 0; // 0 = QR, 1 = Code
    int m_remainingSeconds = 60;
    std::optional<PairingSession> m_session;
    std::unique_ptr<qrcodegen::QrCode> m_qrCode;
    HBITMAP m_hQrBitmap = nullptr;
};

} // namespace AnshuBio
