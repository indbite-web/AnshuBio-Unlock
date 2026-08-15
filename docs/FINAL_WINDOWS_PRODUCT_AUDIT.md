# AnshuBio Unlock — Final Windows Product Audit

**Product:** AnshuBio Unlock  
**Publisher:** AnshuCore  
**Version:** 1.0.0  
**Application ID:** `com.anshucore.bio`  
**Architecture:** Native C++17 / Windows SDK / MinGW-w64 GCC 14.2.0 (UCRT)  
**Target Platform:** Windows 10 & Windows 11 (64-Bit)  

---

## 1. End-to-End Component Audit Matrix

| # | Component / Flow | Status | Verification Detail |
| :---: | :--- | :---: | :--- |
| **1** | **Native C++ Build System** | **PASS** | CMake + Ninja builds `AnshuBioUnlock.exe`, `AnshuBioUnlockService.exe`, `AnshuBioCredentialProvider.dll`, `AnshuBioSessionMonitor.exe`, and `AnshuBioTests.exe` cleanly with 0 compilation errors. |
| **2** | **Production Installer** | **PASS** | Inno Setup 6.7.3 generates `dist/AnshuBioUnlock-Setup.exe` with administrative SCM service registration, Windows firewall rules, and System32 COM DLL registration. |
| **3** | **Uninstaller Cleanup** | **PASS** | Stops and deletes `AnshuBioUnlockService`, removes Credential Provider GUID from `HKLM` & `HKCR`, deletes firewall rules, and purges installation directory. |
| **4** | **Desktop GUI Execution** | **PASS** | Native Win32 Subsystem 2 GUI with `-mwindows` link flag. Runs stably without black console windows or sudden process termination. |
| **5** | **Windows 11 Dark Theme** | **PASS** | Professional `#0c0f17` dark palette, Segoe UI typography, rounded cards, left sidebar navigation (1100x720), and 0 emoji glyphs. |
| **6** | **Windows Service Registration** | **PASS** | `AnshuBioUnlockService` registered in SCM with `SERVICE_WIN32_OWN_PROCESS`, `SERVICE_AUTO_START`, and 3s/5s auto-recovery failure actions. |
| **7** | **Service Decoupled Architecture** | **PASS** | Background service exclusively owns Wi-Fi port 42425, Bluetooth RFCOMM, crypto state, and WTS session monitoring. GUI connects via local IPC. |
| **8** | **Service IPC Channel** | **PASS** | Named Pipe server `\\.\pipe\AnshuBioUnlockAuthPipe` with binary packet validation (`PipeAuthPacket`). |
| **9** | **Credential Provider** | **PASS** | COM class `{B36E9B9A-5827-463F-8C37-67AB12E09B10}` implementing `ICredentialProvider` and `ICredentialProviderCredential2` supporting `CPUS_LOGON` and `CPUS_UNLOCK_WORKSTATION`. |
| **10** | **WTS Session Monitoring** | **PASS** | `WTSRegisterSessionNotification` listening for `WTS_SESSION_LOCK`, `WTS_SESSION_UNLOCK`, `WTS_SESSION_LOGON`, and `WTS_SESSION_LOGOFF`. |
| **11** | **Local Wi-Fi Network Transport** | **PASS** | UDP Broadcast beacon on port 42424 (`DISCOVERY_BEACON`) + HTTP JSON RPC on port 42425 (`/api/anshubio`). 100% offline, zero cloud dependencies. |
| **12** | **Bluetooth RFCOMM Transport** | **PASS** | WinSock Bluetooth Service Class GUID `{1b7e8251-2877-41c3-b46e-cf057c562023}` and SPP channel discovery. |
| **13** | **Mathematical QR Code Generation** | **PASS** | Nayuki C++ QR engine with Medium Error Correction, quiet zone, and real-time GDI rasterization on the dedicated pairing dialog. |
| **14** | **QR Payload Security Guarantee** | **PASS** | Payload contains protocol, version, `pcId`, `sessionId`, `nonce`, IP, port, Bluetooth UUID, and SHA-256 fingerprint. Contains **zero** passwords, PINs, private keys, or biometric data. |
| **15** | **Manual 6-Digit PIN Pairing** | **PASS** | CSPRNG generated 6-digit PIN linked to 60s pairing session TTL and workstation network details. |
| **16** | **Mutual Handshake Confirmation** | **PASS** | Requires explicit approval on both Android phone and Windows desktop dialog before committing to `KeyStore`. |
| **17** | **Trusted Phone Rules** | **PASS** | Strict maximum 2 trusted phones per PC enforced. Rejects 3rd device with warning banner. |
| **18** | **Device & Key Revocation** | **PASS** | Permanently blacklists phone UUID and public key PEM in `KeyStore`. Revoked devices are blocked from authenticating or re-pairing. |
| **19** | **Manual Lock Command** | **PASS** | Authenticated cryptographic phone command executes native `LockWorkStation()` with 30s replay window defense. |
| **20** | **Multiple Workstation Queue** | **PASS** | Sequential challenge processing. Active challenge consumed upon lock resolution. |
| **21** | **No Proximity Auto-Lock Rule** | **PASS** | Wi-Fi/Bluetooth signal loss or disconnect does **not** lock or unlock workstation automatically. |
| **22** | **Power State Lifecycle** | **PASS** | Handles `PBT_APMSUSPEND` (suspends active challenges) and `PBT_APMRESUME` (resumes listeners). |
| **23** | **Windows DPAPI Key Vault** | **PASS** | Windows logon credentials and ECDSA private keys encrypted at rest via `CryptProtectData` (Machine Scope). |
| **24** | **Security Audit Logging** | **PASS** | Thread-safe circular ring buffer written to `%LOCALAPPDATA%\AnshuBio\logs\audit.log` and displayed in the Security Logs view. |
| **25** | **System Tray Integration** | **PASS** | Shell Notify Icon with context menu (`Open AnshuBio Unlock`, `Exit`) and minimize-to-tray support. |
| **26** | **Unit Test Suite** | **PASS** | 33 / 33 Unit Tests passed (100%) covering cryptography, replay defense, DPAPI, rules, and QR protocols. |
| **27** | **Physical Hardware Lock Screen Unlock** | **NOT TESTED** | Hardware validation requires physical Android device with Android BiometricPrompt app communicating over local LAN/Bluetooth to the registered Windows Credential Provider DLL on a live locked Windows workstation. |

---

## 2. Deliverable Verification

```
Distribution Package:     dist/AnshuBioUnlock-Setup.exe
Installation Target:      C:\Program Files\AnshuCore\AnshuBio Unlock\
Main GUI Executable:      AnshuBioUnlock.exe (Subsystem 2: Windows GUI)
Windows Service:          AnshuBioUnlockService.exe (SCM Registered)
Credential Provider:      AnshuBioCredentialProvider.dll (System32 Registered)
Session Monitor:          AnshuBioSessionMonitor.exe
Protocol Spec:            docs/ANSHUBIO_ANDROID_WINDOWS_PROTOCOL.md
Unit Tests:               33 / 33 PASS (100%)
```
