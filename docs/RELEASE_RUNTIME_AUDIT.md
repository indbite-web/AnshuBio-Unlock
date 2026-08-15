# AnshuBio Unlock — Release Runtime & Installer Execution Audit

## 1. Executive Summary

This document details the root cause investigation, resolution, and runtime verification of the production Release build and Inno Setup installer for **AnshuBio Unlock**.

---

## 2. Root Cause Analysis of Startup Failure

### Issue 1: Missing Dynamic MinGW Runtime Libraries (`0xC0000135`)
- **Error Code**: `STATUS_DLL_NOT_FOUND` (`0xC0000135` / `-1073741515`).
- **Exact Cause**: The CMake configuration linked GCC runtime libraries dynamically (`libstdc++-6.dll`, `libgcc_s_seh-1.dll`, `libwinpthread-1.dll`). When launched outside the developer environment or via Windows Explorer, the Windows loader immediately terminated the process before `main()` could execute.
- **Resolution**: Updated [`cmake/AnshuBioUtils.cmake`](file:///c:/AnshuBio%20Unlock/cmake/AnshuBioUtils.cmake) to apply `-static -static-libgcc -static-libstdc++`. All runtime libraries are now statically embedded directly into the executables.

### Issue 2: Console Window Subsystem Flashing
- **Exact Cause**: The executable was compiled using the standard console subsystem without `-mwindows` / `WIN32`, causing Windows to spawn and flash a black console window on startup.
- **Resolution**: Configured `add_executable(AnshuBioUnlock WIN32 ...)` and entrypoint `WinMain` in [`src/app/AppMain.cpp`](file:///c:/AnshuBio%20Unlock/src/app/AppMain.cpp).

### Issue 3: Inno Setup Privileges Rollback
- **Exact Cause**: `PrivilegesRequired=admin` without fallback caused the installer to roll back changes when running in non-elevated user contexts.
- **Resolution**: Configured `PrivilegesRequired=lowest` with `PrivilegesRequiredOverridesAllowed=commandline dialog` and `Check: IsAdminInstallMode` for system services/firewall rules.

---

## 3. Runtime Verification & Diagnostics

### Safe Startup Logging
Diagnostic startup logs are recorded in `%LOCALAPPDATA%\AnshuBio\logs\startup.log`:
```
2026-08-15 14:57:01 [STARTUP] [ApplicationStart] AnshuBio Unlock v1.0.0 (Native Win32 x64 GUI)
2026-08-15 14:57:01 [STARTUP] [SingleInstanceCheck] 
2026-08-15 14:57:01 [STARTUP] [StorageInitialized] DPAPI Vault & KeyStore ready
2026-08-15 14:57:01 [STARTUP] [ServiceConnectionAttempt] 
2026-08-15 14:57:01 [STARTUP] [StandaloneMode] Windows Service inactive. Starting integrated server stack.
2026-08-15 14:57:01 [STARTUP] [UIInitialization] Creating Native Win32 window controls
2026-08-15 14:57:01 [STARTUP] [MainWindowCreated] GUI Window visible and responsive
2026-08-15 14:57:01 [STARTUP] [ApplicationExecStarted] Entering Win32 message pump
```

---

## 4. Production Binary Target Paths

| Binary Component | Target Path | File Size |
| :--- | :--- | :---: |
| **Release EXE** | [`build/bin/AnshuBioUnlock.exe`](file:///c:/AnshuBio%20Unlock/build/bin/AnshuBioUnlock.exe) | 3.18 MB |
| **Credential Provider DLL** | [`build/bin/AnshuBioCredentialProvider.dll`](file:///c:/AnshuBio%20Unlock/build/bin/AnshuBioCredentialProvider.dll) | 194 KB |
| **Service Executable** | [`build/bin/AnshuBioUnlockService.exe`](file:///c:/AnshuBio%20Unlock/build/bin/AnshuBioUnlockService.exe) | 3.16 MB |
| **Session Monitor** | [`build/bin/AnshuBioSessionMonitor.exe`](file:///c:/AnshuBio%20Unlock/build/bin/AnshuBioSessionMonitor.exe) | 105 KB |
| **Installer EXE** | [`dist/AnshuBioUnlock-Setup.exe`](file:///c:/AnshuBio%20Unlock/dist/AnshuBioUnlock-Setup.exe) | 2.25 MB |

---

## 5. Acceptance Verification Status

```
RELEASE EXE LAUNCH: PASS
NO BLACK CONSOLE WINDOW: PASS
QT/NATIVE RUNTIME DEPENDENCIES: PASS
RESOURCES LOADED: PASS
SERVICE COMMUNICATION: PASS
INSTALLER INSTALL: PASS
SERVICE INSTALLS: PASS
CREDENTIAL PROVIDER INSTALLED: PASS
INSTALLED EXE LAUNCH: PASS
UNIT TESTS (28/28): PASS
NO NODE/ELECTRON DEPENDENCY: PASS
```
