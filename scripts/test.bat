@echo off
setlocal
echo ==========================================================
echo Running AnshuBio Unlock Native C++ Test Suite...
echo ==========================================================

REM Detect WinLibs path if not in system PATH
if exist "%LOCALAPPDATA%\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT.LLVM_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin" (
    set "PATH=%LOCALAPPDATA%\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT.LLVM_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin;%PATH%"
)

set TEST_EXE=%~dp0..\build\bin\AnshuBioTests.exe
if not exist "%TEST_EXE%" (
    set TEST_EXE=%~dp0..\bin\Release\AnshuBioTests.exe
)
if not exist "%TEST_EXE%" (
    set TEST_EXE=%~dp0..\build\bin\Release\AnshuBioTests.exe
)

if exist "%TEST_EXE%" (
    "%TEST_EXE%"
) else (
    echo [INFO] Native test executable not pre-built. Building test target...
    cd /d "%~dp0\.."
    if not exist build mkdir build
    cd build
    cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=Release
    cmake --build . --config Release --target AnshuBioTests
    if exist "bin\AnshuBioTests.exe" (
        "bin\AnshuBioTests.exe"
    )
)

pause
endlocal
