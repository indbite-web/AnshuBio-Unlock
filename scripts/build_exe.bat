@echo off
setlocal
echo ==========================================================
echo Building Native AnshuBio Unlock Windows Executable...
echo Architecture: C++17 / Qt 6 / CMake / Windows SDK
echo Publisher: AnshuCore
echo ==========================================================

cd /d "%~dp0\.."

REM Detect WinLibs path if not in system PATH
if exist "%LOCALAPPDATA%\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT.LLVM_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin" (
    set "PATH=%LOCALAPPDATA%\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT.LLVM_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin;%PATH%"
)

if not exist build mkdir build
cd build

cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% NEQ 0 (
    cmake .. -G "Visual Studio 17 2022" -A x64
    if %ERRORLEVEL% NEQ 0 (
        cmake ..
    )
)

cmake --build . --config Release
if %ERRORLEVEL% EQU 0 (
    echo.
    echo ==========================================================
    echo [SUCCESS] Native AnshuBio Unlock binaries built successfully!
    echo Application: %~dp0..\build\bin\AnshuBioUnlock.exe
    echo Tests:       %~dp0..\build\bin\AnshuBioTests.exe
    echo CP DLL:      %~dp0..\build\bin\AnshuBioCredentialProvider.dll
    echo Monitor:     %~dp0..\build\bin\AnshuBioSessionMonitor.exe
    echo ==========================================================
) else (
    echo.
    echo [ERROR] Native build failed with exit code %ERRORLEVEL%
)

endlocal
