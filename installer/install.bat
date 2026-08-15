@echo off
setlocal
echo ==========================================================
echo        AnshuBio Unlock - Windows Production Installer
echo                     Publisher: AnshuCore
echo ==========================================================

:: Check admin privileges
net session >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Administrator permissions are required to install AnshuBio Unlock.
    echo Please right-click this script and choose "Run as administrator".
    pause
    exit /b 1
)

set TARGET_DIR=%ProgramFiles%\AnshuCore\AnshuBio Unlock
set BIN_DIR=%~dp0..\bin\Release
if not exist "%BIN_DIR%\AnshuBioUnlock.exe" (
    set BIN_DIR=%~dp0..\build\bin\Release
)
if not exist "%BIN_DIR%\AnshuBioUnlock.exe" (
    set BIN_DIR=%~dp0..\build\bin
)

echo [STEP 1/5] Creating application directory...
if not exist "%TARGET_DIR%" mkdir "%TARGET_DIR%"

echo [STEP 2/5] Deploying native AnshuBio Unlock binaries...
if not exist "%BIN_DIR%\AnshuBioUnlock.exe" (
    echo [ERROR] Native compiled application not found at:
    echo         %BIN_DIR%\AnshuBioUnlock.exe
    echo.
    echo Please build the application first by running:
    echo         scripts\build_exe.bat
    echo.
    pause
    exit /b 1
)

echo Copying application package to %TARGET_DIR%...
xcopy /E /I /Y "%BIN_DIR%\*" "%TARGET_DIR%\" >nul

echo [STEP 3/5] Configuring Windows Startup Run key...
reg add "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v "AnshuBioUnlock" /t REG_SZ /d "\"%TARGET_DIR%\AnshuBioUnlock.exe\" --background" /f >nul 2>&1

echo [STEP 4/5] Registering Windows Credential Provider...
call "%~dp0register_cp.bat"

echo [STEP 5/5] Creating Start Menu and Desktop shortcuts...
set START_MENU_DIR=%ProgramData%\Microsoft\Windows\Start Menu\Programs\AnshuBio Unlock
if not exist "%START_MENU_DIR%" mkdir "%START_MENU_DIR%"

:: Create Start Menu shortcut using PowerShell
powershell -NoProfile -ExecutionPolicy Bypass -Command "$ws = New-Object -ComObject WScript.Shell; $s = $ws.CreateShortcut('%START_MENU_DIR%\\AnshuBio Unlock.lnk'); $s.TargetPath = '%TARGET_DIR%\\AnshuBioUnlock.exe'; $s.WorkingDirectory = '%TARGET_DIR%'; $s.Description = 'AnshuBio Unlock - Local Phone Biometric PC Unlock'; $s.Save()" >nul 2>&1

:: Create Desktop shortcut
powershell -NoProfile -ExecutionPolicy Bypass -Command "$ws = New-Object -ComObject WScript.Shell; $s = $ws.CreateShortcut([Environment]::GetFolderPath('Desktop') + '\\AnshuBio Unlock.lnk'); $s.TargetPath = '%TARGET_DIR%\\AnshuBioUnlock.exe'; $s.WorkingDirectory = '%TARGET_DIR%'; $s.Description = 'AnshuBio Unlock'; $s.Save()" >nul 2>&1

echo ==========================================================
echo [SUCCESS] AnshuBio Unlock has been installed successfully!
echo Executable: %TARGET_DIR%\AnshuBioUnlock.exe
echo Standard Windows Password and PIN login remain fully operational.
echo ==========================================================
pause
endlocal
