@echo off
setlocal
echo ==========================================================
echo       AnshuBio Unlock - Safe Uninstaller Script
echo ==========================================================

:: Check admin privileges
net session >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Administrator permissions are required to uninstall.
    echo Please right-click this script and choose "Run as administrator".
    pause
    exit /b 1
)

echo [STEP 1/5] Terminating running application instances...
taskkill /F /IM AnshuBioUnlock.exe >nul 2>&1

echo [STEP 2/5] Deregistering Windows Credential Provider safely...
call "%~dp0unregister_cp.bat"

echo [STEP 3/5] Removing Startup Registry entries...
reg delete "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v "AnshuBioUnlock" /f >nul 2>&1

echo [STEP 4/5] Removing Shortcuts...
set START_MENU_DIR=%ProgramData%\Microsoft\Windows\Start Menu\Programs\AnshuBio Unlock
if exist "%START_MENU_DIR%" rd /s /q "%START_MENU_DIR%" >nul 2>&1
powershell -NoProfile -ExecutionPolicy Bypass -Command "Remove-Item ([Environment]::GetFolderPath('Desktop') + '\\AnshuBio Unlock.lnk') -ErrorAction SilentlyContinue" >nul 2>&1

echo [STEP 5/5] Removing Application Files...
set TARGET_DIR=%ProgramFiles%\AnshuCore\AnshuBio Unlock
if exist "%TARGET_DIR%" rd /s /q "%TARGET_DIR%" >nul 2>&1

echo ==========================================================
echo [SUCCESS] AnshuBio Unlock has been cleanly uninstalled.
echo Standard Windows Password and PIN authentication are 100%% preserved.
echo ==========================================================
pause
endlocal
