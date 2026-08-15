# AnshuBio Unlock — Reference Architecture Analysis

**Reference Project**: PC Bio Unlock Desktop v3.3.4  
**Target Product**: AnshuBio Unlock v1.0.0  
**Publisher**: AnshuCore  
**Target Architecture**: Native C++ / Qt 6 / CMake / Windows SDK  
**Identity**: `com.anshucore.bio` (Windows & Android Companion)  
**Date**: August 15, 2026  

---

## 1. Executive Summary & Purpose

This document provides a deep architectural and technical analysis of the PC Bio Unlock v3.3.4 desktop reference project located at `C:\Anshu\pcbu-desktop-3.3.4`. 

The reference codebase was studied strictly as an architectural reference to understand native Windows Credential Provider integration, Bluetooth RFCOMM sockets, Wi-Fi networking, session event dispatching, and Qt desktop GUI patterns.

> [!IMPORTANT]
> **Clean-Room & Intellectual Property Notice:**
> - No branding, assets, product names, or proprietary identifiers are copied from the reference project.
> - AnshuBio Unlock is a clean-room native implementation with its own distinct cryptographic model (hardware-backed Android Keystore ECDSA signatures, machine-bound DPAPI encrypted vault, AirPods-style compact floating phone UI, and strict zero-biometric-transmission security boundaries).

---

## 2. Key Architectural Patterns Found in Reference

### 2.1 Windows Credential Provider Subsystem (`natives/win-pcbiounlock/`)
- **COM Class Architecture**: Implements `ICredentialProvider`, `ICredentialProviderCredential`, `ICredentialProviderCredential2`, and `ICredentialProviderFilter`.
- **LSA Package Resolution**: Uses `RetrieveNegotiateAuthPackage()` via `LsaConnectUntrusted` and `LsaLookupAuthenticationPackage` with `NEGOSSP_NAME_A` ("Negotiate") to support local accounts, Active Directory, and Microsoft Accounts seamlessly.
- **Logon Serialization Structure**: Uses `KERB_INTERACTIVE_UNLOCK_LOGON` / `KERB_INTERACTIVE_LOGON` packed with relative buffer offsets via `KerbInteractiveUnlockLogonInit()` and `KerbInteractiveUnlockLogonPack()`.
- **Credential Protection**: Uses `CredProtectW` (`ProtectIfNecessaryAndCopyPassword()`) for secure memory handling before passing serialization to Windows LogonUI.
- **IPC Listener (`CUnlockListener`)**: Runs a background listener thread inside the Credential Provider instance waiting for authentication signals from the background service.

### 2.2 Native Windows Bluetooth RFCOMM Transport (`common/src/connection/unlock/servers/BTUnlockServer.cpp` & `BluetoothHelper.Win.cpp`)
- **Winsock Bluetooth Sockets**: Uses native Windows `AF_BTH`, `SOCK_STREAM`, and `BTHPROTO_RFCOMM` (`ws2bth.h`).
- **SDP Service Registration**: Uses `WSASetServiceW` to publish the custom RFCOMM service UUID on the local Bluetooth radio.
- **Device Enumeration & Pairing**: Uses Win32 `BluetoothFindFirstRadio`, `BluetoothFindFirstDevice`, and `BluetoothAuthenticateDevice` (`bthprops.lib`).

### 2.3 Local Wi-Fi & Offline Network Transport (`common/src/connection/`)
- **UDP Discovery**: Broadcast beacon on UDP port 42424 with JSON discovery packets.
- **TCP/HTTP/WebSocket Unlock Server**: Offline socket server on TCP port 42425 for pairing, challenge-response authentication, and state sync. Zero cloud or external dependencies.

### 2.4 Windows Service & Installer (`desktop/src/installer/ServiceInstaller.Win.cpp`)
- **DLL Deployment**: Installs the Credential Provider DLL into `%SystemRoot%\System32` (or application directory).
- **Registry Integration**: Configures `HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Authentication\Credential Providers\{GUID}` and `HKCR\CLSID\{GUID}\InprocServer32` with `Apartment` threading model.
- **Windows Firewall**: Adds authorized rules using Windows Firewall COM APIs (`INetFwPolicy2`).

### 2.5 Qt Desktop Application Architecture (`desktop/`)
- **Qt Core / GUI / Network**: Structured C++ application using `QApplication`, `QSystemTrayIcon`, `QMainWindow`, and `QTranslator`.

---

## 3. What Will Be Adopted for AnshuBio Unlock

1. **Native C++ Windows Credential Provider**: Standard `ICredentialProviderCredential2` implementation with `RetrieveNegotiateAuthPackage()` and `KERB_INTERACTIVE_UNLOCK_LOGON` serialization.
2. **Native Win32 Session Notification**: `WTSRegisterSessionNotification` for real `WTS_SESSION_LOCK`, `WTS_SESSION_UNLOCK`, `WTS_SESSION_LOGON`, and `WTS_SESSION_LOGOFF`.
3. **Winsock Bluetooth RFCOMM Architecture**: Real Windows Bluetooth server (`AF_BTH` / `BTHPROTO_RFCOMM`) alongside local offline Wi-Fi LAN transport with automatic failover.
4. **Native C++ / Qt 6 Desktop UI**: High-performance, memory-efficient native GUI with dark theme, Material Icons, system tray management, setup wizard, and real-time security logging.
5. **Production Inno Setup Installer**: Native setup executable (`AnshuBioUnlock-Setup.exe`) deploying binaries, service, and registering COM Credential Provider cleanly.

---

## 4. What Will NOT Be Copied & Critical Security Differences

| Aspect | Reference Implementation | AnshuBio Unlock Native Architecture |
|---|---|---|
| **Product Branding** | PC Bio Unlock / Meis Apps | **AnshuBio Unlock / AnshuCore** |
| **Identifiers** | `meis.pcbiounlock` | **`com.anshucore.bio`** |
| **GUI Framework** | QML / Custom styles | **Qt 6 Native Widgets / QML Modern Windows 11 Acrylic Glassmorphism** |
| **Android Biometric Model** | Custom / legacy templates | **Strict Android Native `BiometricPrompt` & TEE Keystore (Zero biometric template transmission)** |
| **Device Limits** | Paid/Pro upgrade limitations | **100% Free & Unlimited PCs per phone; Strict max 2 phones per PC for security** |
| **Android Popup UX** | Full-screen app | **AirPods-style compact floating bottom sheet card** |
| **Credential Storage** | Custom password vault | **Machine-bound AES-256-GCM vault protected by Windows DPAPI** |
| **Icons & Theme** | Mixed icons & emojis | **Strict Material Icons ONLY, Dark Theme ONLY, English ONLY, NO Emojis** |

---

## 5. Security Architecture Bridge

AnshuBio Unlock enforces a strict zero-knowledge security boundary:

```
+-------------------------------------------------------------+
|                 Android Phone (AnshuBio App)                |
|   1. Receives 32-byte CSPRNG challenge from PC             |
|   2. Invokes native BiometricPrompt (Fingerprint/Face/PIN)  |
|   3. Hardware TEE Keystore signs challenge                  |
|   4. Transmits ONLY ECDSA signature proof to PC             |
+-------------------------------------------------------------+
                              |
                     [Local Wi-Fi / BT]
                              |
                              v
+-------------------------------------------------------------+
|               Windows PC (AnshuBio Core Daemon)             |
|   1. Verifies ECDSA signature against trusted public key    |
|   2. Validates single-use nonce & timestamp window          |
|   3. Retrieves DPAPI-decrypted user credential              |
|   4. Bridges credential to Named Pipe for LogonUI           |
+-------------------------------------------------------------+
                              |
                        [Named Pipe]
                              |
                              v
+-------------------------------------------------------------+
|          Windows Credential Provider (DLL in LogonUI)       |
|   1. Packs KERB_INTERACTIVE_UNLOCK_LOGON buffer             |
|   2. Passes to LsaLogonUser via Negotiate Package           |
|   3. Performs SecureZeroMemory on all buffers               |
|   4. Detects actual WTS_SESSION_UNLOCK from Windows OS      |
+-------------------------------------------------------------+
```
