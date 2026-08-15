cmake_minimum_required(VERSION 3.16)
project(AnshuBioUnlock VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(AnshuBioUtils)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Find Qt 6 Packages
find_package(Qt6 COMPONENTS Core Gui Widgets Network QUIET)
if (Qt6_FOUND)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
    set(CMAKE_AUTOUIC ON)
endif()

# Core sources
set(CORE_SOURCES
    src/core/Constants.h
    src/core/Models.h
    src/core/AuthCoordinator.h
    src/core/AuthCoordinator.cpp
    src/crypto/DPAPIVault.h
    src/crypto/DPAPIVault.cpp
    src/crypto/CryptoEngine.h
    src/crypto/CryptoEngine.cpp
    src/storage/SecurityLogger.h
    src/storage/SecurityLogger.cpp
    src/storage/KeyStore.h
    src/storage/KeyStore.cpp
    src/session/SessionMonitor.h
    src/session/SessionMonitor.cpp
    src/service/NamedPipeServer.h
    src/service/NamedPipeServer.cpp
    src/service/WindowsService.h
    src/service/WindowsService.cpp
    src/networking/wifi/UDPBeacon.h
    src/networking/wifi/UDPBeacon.cpp
    src/networking/wifi/WiFiServer.h
    src/networking/wifi/WiFiServer.cpp
    src/networking/bluetooth/BluetoothHelper.h
    src/networking/bluetooth/BluetoothHelper.cpp
    src/networking/bluetooth/BluetoothServer.h
    src/networking/bluetooth/BluetoothServer.cpp
)

# UI sources (Qt 6)
set(UI_SOURCES
    src/ui/Theme.h
    src/ui/DashboardWidget.h
    src/ui/DashboardWidget.cpp
    src/ui/TrustedPhonesWidget.h
    src/ui/TrustedPhonesWidget.cpp
    src/ui/SecurityWidget.h
    src/ui/SecurityWidget.cpp
    src/ui/SettingsWidget.h
    src/ui/SettingsWidget.cpp
    src/ui/LogsWidget.h
    src/ui/LogsWidget.cpp
    src/ui/AboutWidget.h
    src/ui/AboutWidget.cpp
    src/ui/SetupWizard.h
    src/ui/SetupWizard.cpp
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
)

if (Qt6_FOUND)
    add_executable(AnshuBioUnlock WIN32
        src/app/main.cpp
        ${CORE_SOURCES}
        ${UI_SOURCES}
        resources/app.rc
    )

    target_include_directories(AnshuBioUnlock PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    target_link_libraries(AnshuBioUnlock PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Network
        ws2_32
        crypt32
        bcrypt
        rpcrt4
        wtsapi32
        user32
        advapi32
        bthprops
    )

    anshubio_apply_security_flags(AnshuBioUnlock)
else()
    # Native Windows Application Launcher / CLI Target
    add_executable(AnshuBioUnlock
        src/service/ServiceMain.cpp
        ${CORE_SOURCES}
    )

    target_include_directories(AnshuBioUnlock PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    target_link_libraries(AnshuBioUnlock PRIVATE
        ws2_32
        crypt32
        bcrypt
        rpcrt4
        wtsapi32
        user32
        advapi32
        bthprops
    )

    anshubio_apply_security_flags(AnshuBioUnlock)
endif()

# Standalone Windows Background Service Target
add_executable(AnshuBioUnlockService
    src/service/ServiceMain.cpp
    ${CORE_SOURCES}
)

target_include_directories(AnshuBioUnlockService PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(AnshuBioUnlockService PRIVATE
    ws2_32
    crypt32
    bcrypt
    rpcrt4
    wtsapi32
    user32
    advapi32
    bthprops
)

anshubio_apply_security_flags(AnshuBioUnlockService)

# Credential Provider Target
add_subdirectory(credential_provider)

# Native Unit Tests Target
add_executable(AnshuBioTests
    tests/TestRunner.cpp
    ${CORE_SOURCES}
)

target_include_directories(AnshuBioTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(AnshuBioTests PRIVATE
    ws2_32
    crypt32
    bcrypt
    rpcrt4
    wtsapi32
    user32
    advapi32
    bthprops
)

anshubio_apply_security_flags(AnshuBioTests)
