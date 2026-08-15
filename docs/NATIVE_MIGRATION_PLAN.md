# AnshuBio Unlock — Native C++ / Qt 6 Migration Plan

**Product**: AnshuBio Unlock  
**Publisher**: AnshuCore  
**Target Architecture**: Native C++17 / Qt 6 / CMake / Windows SDK  
**Identity**: `com.anshucore.bio`  
**Date**: August 15, 2026  

---

## 1. Migration Overview

This plan outlines the complete migration of AnshuBio Unlock from the JavaScript/Node/Electron prototype into a **100% Native Windows Application Suite** built entirely with **C++ / Qt 6 / CMake / Windows SDK**.

---

## 2. Component Mapping Table

| Prototype Component (JS/Node) | Native C++ / Qt 6 Replacement | Implementation Details |
|---|---|---|
| `src/ui/main_electron.js` | `src/app/main.cpp` & `src/ui/MainWindow.cpp` | Native Qt 6 GUI application with `QApplication`, `QSystemTrayIcon`, Acrylic dark palette, Material Icons, and setup wizard. |
| `src/ui/app.js` & `styles.css` | `src/ui/DashboardWidget.cpp`, `TrustedPhonesWidget.cpp`, `SecurityWidget.cpp`, `SettingsWidget.cpp`, `LogsWidget.cpp`, `SetupWizard.cpp` | Native Qt 6 widgets with dark stylesheet, smooth QPropertyAnimations, and zero web engine dependencies. |
| `src/service/auth_coordinator.js` | `src/core/AuthCoordinator.cpp` | Central C++ state machine managing authentication challenges, 32-byte nonces, multi-PC queuing, and device pairing. |
| `src/service/session_monitor.js` & `native_session_monitor.js` | `src/session/SessionMonitor.cpp` | Native Win32 message loop calling `WTSRegisterSessionNotification` and handling `WM_WTSSESSION_CHANGE` and `WM_POWERBROADCAST`. |
| `src/service/wifi_server.js` | `src/networking/wifi/WiFiServer.cpp` | Native C++ Winsock / QtNetwork UDP discovery beacon (port 42424) and offline TCP/HTTP/WebSocket server (port 42425). |
| `src/service/ble_server.js` | `src/networking/bluetooth/BluetoothServer.cpp` | Native Windows Bluetooth RFCOMM server (`AF_BTH`, `BTHPROTO_RFCOMM`, `WSASetServiceW`, `BluetoothFindFirstRadio`). |
| `src/service/pipe_server.js` | `src/service/NamedPipeServer.cpp` | Win32 Named Pipe server (`\\.\pipe\AnshuBioUnlockAuthPipe`) connecting Credential Provider DLL and desktop UI. |
| `src/crypto/crypto_engine.js` | `src/crypto/CryptoEngine.cpp` | OpenSSL / Windows CNG ECDSA P-256 key generation, SHA-256 signatures, CSPRNG challenge generation, and single-use nonce tracking. |
| `src/crypto/key_store.js` | `src/storage/KeyStore.cpp` | Machine-bound AES-256-GCM encrypted vault using Windows DPAPI (`CryptProtectData` / `CryptUnprotectData`). |
| `src/credential_provider/` | `credential_provider/` | Native C++ COM DLL implementing `ICredentialProviderCredential2`, `RetrieveNegotiateAuthPackage()`, and `KERB_INTERACTIVE_UNLOCK_LOGON`. |
| `installer/` | `installer/installer.iss` & `installer/install.bat` | Production Inno Setup script generating `AnshuBioUnlock-Setup.exe` with COM registration and safe uninstall rollback. |

---

## 3. Native Project Directory Structure

```
AnshuBioUnlock/
├── CMakeLists.txt
├── cmake/
│   └── AnshuBioUtils.cmake
├── src/
│   ├── app/
│   │   ├── main.cpp
│   │   └── AppController.cpp / .h
│   ├── ui/
│   │   ├── MainWindow.cpp / .h
│   │   ├── Theme.h
│   │   ├── DashboardWidget.cpp / .h
│   │   ├── TrustedPhonesWidget.cpp / .h
│   │   ├── SecurityWidget.cpp / .h
│   │   ├── SettingsWidget.cpp / .h
│   │   ├── LogsWidget.cpp / .h
│   │   ├── AboutWidget.cpp / .h
│   │   ├── SetupWizard.cpp / .h
│   │   └── SystemTray.cpp / .h
│   ├── core/
│   │   ├── AuthCoordinator.cpp / .h
│   │   ├── Constants.h
│   │   └── Models.h
│   ├── crypto/
│   │   ├── CryptoEngine.cpp / .h
│   │   └── DPAPIVault.cpp / .h
│   ├── networking/
│   │   ├── wifi/
│   │   │   ├── WiFiServer.cpp / .h
│   │   │   └── UDPBeacon.cpp / .h
│   │   └── bluetooth/
│   │       ├── BluetoothServer.cpp / .h
│   │       └── BluetoothHelper.cpp / .h
│   ├── session/
│   │   └── SessionMonitor.cpp / .h
│   ├── storage/
│   │   ├── KeyStore.cpp / .h
│   │   └── SecurityLogger.cpp / .h
│   └── service/
│       ├── NamedPipeServer.cpp / .h
│       └── WindowsService.cpp / .h
├── credential_provider/
│   ├── CMakeLists.txt
│   ├── AnshuBioCredentialProvider.cpp / .h
│   ├── AnshuBioCredential.cpp / .h
│   ├── Dll.cpp
│   ├── guid.h
│   └── AnshuBioCredentialProvider.def
├── installer/
│   ├── installer.iss
│   ├── install.bat
│   └── uninstall.bat
├── resources/
│   ├── icons/
│   └── app.rc
├── tests/
│   ├── CMakeLists.txt
│   ├── CryptoTests.cpp
│   ├── SessionTests.cpp
│   └── ProtocolTests.cpp
└── docs/
    ├── REFERENCE_ARCHITECTURE_ANALYSIS.md
    └── NATIVE_MIGRATION_PLAN.md
```

---

## 4. Phased Implementation Roadmap

1. **Phase 1: Core Engine & Cryptography**: Native C++ DPAPI vault, NIST P-256 ECDSA verification, challenge generator, and security logger.
2. **Phase 2: Networking & Transports**: Native Winsock Wi-Fi UDP beacon & TCP/HTTP server, and native Windows Bluetooth RFCOMM socket server.
3. **Phase 3: Session Monitoring & Named Pipe IPC**: Native Win32 WTS notification pump and secure named pipe server.
4. **Phase 4: Credential Provider DLL**: Full `KERB_INTERACTIVE_UNLOCK_LOGON` / `Negotiate` package serialization with `SecureZeroMemory` scrubbing.
5. **Phase 5: Native Qt 6 Desktop UI**: Acrylic dark theme, Material Icons, Dashboard, Trusted Phones, Security, Settings, Logs, About, and Setup Wizard.
6. **Phase 6: Native Installer**: Inno Setup script compiling `AnshuBioUnlock-Setup.exe`.
