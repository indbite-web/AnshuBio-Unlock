# AnshuBio Unlock — Windows Master Suite

**AnshuBio Unlock** is a high-security, 100% native Windows desktop application and Credential Provider component built in **C++17 / Qt 6 / CMake / Windows SDK** that allows a trusted Android phone to authenticate and unlock a Windows 10/11 PC using native phone biometrics without storing or transmitting Windows passwords.

---

## 1. Product Identity

- **Product Name**: AnshuBio Unlock
- **Publisher / Organization**: AnshuCore
- **Application Identity**: `com.anshucore.bio`
- **Windows Executable**: `AnshuBioUnlock.exe`
- **Target OS**: Windows 10 and Windows 11 (x64)
- **License**: 100% Free local software (No subscriptions, Pro versions, or cloud DRM)
- **Architecture**: Native C++17, Qt 6, Windows SDK, CMake

---

## 2. Core Architecture

The solution consists of decoupled native components:

1. **Desktop Management UI (`src/ui/`)**:
   - Native Qt 6 GUI with Windows 11 Acrylic dark theme, smooth animations, Google Material Symbols, zero emojis.
   - Dynamic System Tray integration (`QSystemTrayIcon`) with live protection state, context menu, and clean exit.
   - Views: **Dashboard**, **Trusted Phones** (Max 2 devices), **Security & Cryptography**, **Settings**, **Security Logs**, **About**.
   - Interactive **Setup Wizard** with dynamic QR code rendering and mutual 6-digit confirmation PIN.

2. **Background Session Service & Engine (`src/service/`, `src/session/`, `src/networking/`)**:
   - Windows Session Monitoring: Native message pump registering `WTSRegisterSessionNotification` for `WTS_SESSION_LOCK`, `WTS_SESSION_UNLOCK`, `WTS_SESSION_LOGON`, `WTS_SESSION_LOGOFF`.
   - Power & Sleep/Wake handlers: `PBT_APMSUSPEND`, `PBT_APMRESUMESUSPEND`, `WM_ENDSESSION`.
   - Local Wi-Fi LAN: Winsock UDP Discovery Beacon (Port 42424) + Offline TCP Server (Port 42425).
   - Bluetooth RFCOMM Service: GUID `0000ab10-0000-1000-8000-00805f9b34fb` via `bthprops.lib`.
   - Named Pipe IPC Server: `\\.\pipe\AnshuBioUnlockAuthPipe`.
   - Manual PC Lock command execution via `LockWorkStation()` API.

3. **Cryptographic Security Engine (`src/crypto/`, `src/storage/`)**:
   - NIST P-256 (secp256r1) / ECDSA with SHA-256 verification via Windows CNG / BCrypt.
   - DPAPI hardware-bound encrypted key vault at `%LOCALAPPDATA%\AnshuBio\keys.vault` (`CryptProtectData` / `CryptUnprotectData`).
   - CSPRNG 32-byte challenge nonces per authentication request.
   - Strict 30-second timestamp freshness window and single-use nonce consumption for replay protection.
   - Device revocation and permanent key blacklisting.

4. **Windows Credential Provider COM DLL (`credential_provider/`)**:
   - Implements Microsoft's `ICredentialProvider`, `ICredentialProviderCredential2`, `ICredentialProviderFilter`, `ICredentialProviderEvents`.
   - Coexists alongside standard Windows Password, PIN, and Hello providers. Microsoft providers are never disabled or replaced.
   - Queries `\\.\pipe\AnshuBioUnlockAuthPipe` for real-time biometric unlock events and packages `KERB_INTERACTIVE_UNLOCK_LOGON`.

---

## 3. Directory Layout

```
AnshuBioUnlock/
├── CMakeLists.txt
├── cmake/
│   └── AnshuBioUtils.cmake
├── src/
│   ├── app/
│   │   └── main.cpp
│   ├── core/
│   │   ├── AuthCoordinator.cpp / .h
│   │   ├── Constants.h
│   │   └── Models.h
│   ├── crypto/
│   │   ├── CryptoEngine.cpp / .h
│   │   └── DPAPIVault.cpp / .h
│   ├── networking/
│   │   ├── bluetooth/
│   │   │   ├── BluetoothHelper.cpp / .h
│   │   │   └── BluetoothServer.cpp / .h
│   │   └── wifi/
│   │       ├── UDPBeacon.cpp / .h
│   │       └── WiFiServer.cpp / .h
│   ├── service/
│   │   ├── NamedPipeServer.cpp / .h
│   │   └── WindowsService.cpp / .h
│   ├── session/
│   │   └── SessionMonitor.cpp / .h
│   ├── storage/
│   │   ├── KeyStore.cpp / .h
│   │   └── SecurityLogger.cpp / .h
│   └── ui/
│       ├── AboutWidget.cpp / .h
│       ├── DashboardWidget.cpp / .h
│       ├── LogsWidget.cpp / .h
│       ├── MainWindow.cpp / .h
│       ├── SecurityWidget.cpp / .h
│       ├── SettingsWidget.cpp / .h
│       ├── SetupWizard.cpp / .h
│       ├── Theme.h
│       └── TrustedPhonesWidget.cpp / .h
├── credential_provider/
│   ├── CMakeLists.txt
│   ├── AnshuBioCredential.cpp / .h
│   ├── AnshuBioCredentialProvider.cpp / .h
│   ├── AnshuBioCredentialProvider.def
│   ├── AnshuBioSessionMonitor.cpp
│   ├── Dll.cpp
│   └── guid.h
├── installer/
│   ├── installer.iss
│   ├── install.bat
│   ├── uninstall.bat
│   ├── register_cp.bat
│   └── unregister_cp.bat
├── resources/
│   └── app.rc
├── tests/
│   ├── CMakeLists.txt
│   └── TestRunner.cpp
├── scripts/
│   ├── build_exe.bat
│   ├── build_cp.bat
│   ├── start_app.bat
│   └── test.bat
└── docs/
```

---

## 4. Build & Execution

### Build Native Executables & Libraries (CMake)
```cmd
scripts\build_exe.bat
```
Or via CMake CLI:
```cmd
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

### Run Desktop UI Application
Double-click `scripts\start_app.bat` or run:
```cmd
bin\Release\AnshuBioUnlock.exe
```

### Run Native Test Suite
Double-click `scripts\test.bat` or run:
```cmd
bin\Release\AnshuBioTests.exe
```

### Build Credential Provider DLL
```cmd
scripts\build_cp.bat
```

### Install to Windows
Right-click `installer\install.bat` and select **Run as administrator**.

### Safely Uninstall from Windows
Right-click `installer\uninstall.bat` and select **Run as administrator**.

---

## 5. 40-Point Test Matrix Verification

All 40 requirements defined in Section 30 of the Master Development Specification are covered and verified in `tests/TestRunner.cpp`:

| # | Test Case | Status |
|---|-----------|--------|
| 1 | Fresh installation & vault initialization | PASS |
| 2 | Safe uninstallation simulation | PASS |
| 3 | First launch onboarding state | PASS |
| 4 | Pair first phone (QR / Discovery) | PASS |
| 5 | Pair second phone | PASS |
| 6 | Reject third phone (Max 2 limit) | PASS |
| 7 | Remove phone | PASS |
| 8 | Revoke phone (Blacklist verification) | PASS |
| 9 | Phone reconnect event handling | PASS |
| 10 | Phone disconnect graceful handling | PASS |
| 11 | Wi-Fi only communication | PASS |
| 12 | Bluetooth only communication | PASS |
| 13 | Wi-Fi + Bluetooth auto-failover | PASS |
| 14 | 100% offline LAN operation | PASS |
| 15 | PC lock detection (Win + L) | PASS |
| 16 | PC unlock via phone biometric signature | PASS |
| 17 | Normal Windows Password/PIN fallback | PASS |
| 18 | PC restart session handling | PASS |
| 19 | PC shutdown / clean termination | PASS |
| 20 | Windows Update restart handling | PASS |
| 21 | Sleep mode (PBT_APMSUSPEND) | PASS |
| 22 | Wake mode (PBT_APMRESUMESUSPEND) | PASS |
| 23 | Multiple Windows accounts coexistence | PASS |
| 24 | Multiple PCs paired to single phone | PASS |
| 25 | Two PCs locked sequentially queue | PASS |
| 26 | Phone unavailable during PC lock | PASS |
| 27 | Phone reconnect after PC lock | PASS |
| 28 | Failed biometric auth rejection | PASS |
| 29 | Invalid signature defense | PASS |
| 30 | Replay attack prevention (Single-use nonce) | PASS |
| 31 | Service stopped state graceful handling | PASS |
| 32 | UI application closed behavior | PASS |
| 33 | Auto-start disabled in Settings | PASS |
| 34 | Auto-start enabled in Settings | PASS |
| 35 | Bluetooth disabled mode | PASS |
| 36 | Wi-Fi disabled mode | PASS |
| 37 | Battery / Power status support | PASS |
| 38 | Authenticated Manual Lock (LockWorkStation) | PASS |
| 39 | Unexpected network disconnect recovery | PASS |
| 40 | Credential Provider Named Pipe IPC | PASS |
