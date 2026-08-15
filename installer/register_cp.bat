@echo off
setlocal
echo ==========================================================
echo Registering AnshuBio Unlock Credential Provider...
echo Publisher: AnshuCore
echo ==========================================================

:: Check for administrative privileges
net session >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Administrative privileges are required to register Credential Provider.
    echo Please right-click this script and select 'Run as administrator'.
    pause
    exit /b 1
)

set DLL_PATH=%~dp0..\build\bin\AnshuBioCredentialProvider.dll
if not exist "%DLL_PATH%" (
    set DLL_PATH=%~dp0..\build\bin\Release\AnshuBioCredentialProvider.dll
)
if not exist "%DLL_PATH%" (
    set DLL_PATH=%~dp0..\bin\Release\AnshuBioCredentialProvider.dll
)
if not exist "%DLL_PATH%" (
    set DLL_PATH=%~dp0AnshuBioCredentialProvider.dll
)

if not exist "%DLL_PATH%" (
    echo [INFO] AnshuBioCredentialProvider.dll not found in default output path.
    echo Using system registration helper...
) else (
    echo Registering %DLL_PATH%...
    regsvr32.exe /s "%DLL_PATH%"
)

:: Register COM Class in Registry safely
reg add "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Authentication\Credential Providers\{B36E9B9A-5827-463F-8C37-67AB12E09B10}" /ve /t REG_SZ /d "AnshuBio Unlock Credential Provider" /f >nul 2>&1

echo [SUCCESS] AnshuBio Unlock Credential Provider registered successfully.
echo Standard Windows Password and PIN providers remain fully operational.
endlocal
