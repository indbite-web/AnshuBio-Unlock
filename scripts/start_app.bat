@echo off
setlocal
echo Starting Native AnshuBio Unlock...

set EXE_PATH=%~dp0..\bin\Release\AnshuBioUnlock.exe
if not exist "%EXE_PATH%" (
    set EXE_PATH=%~dp0..\build\bin\Release\AnshuBioUnlock.exe
)
if not exist "%EXE_PATH%" (
    set EXE_PATH=%~dp0..\build\bin\AnshuBioUnlock.exe
)

if exist "%EXE_PATH%" (
    start "" "%EXE_PATH%" %*
) else (
    echo [ERROR] AnshuBioUnlock.exe not found.
    echo Please build the application first by running: scripts\build_exe.bat
    pause
)

endlocal
