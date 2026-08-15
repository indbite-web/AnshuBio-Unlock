cmake_minimum_required(VERSION 3.20)
project(AnshuBioCredentialProvider LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-DUNICODE -D_UNICODE -DWIN32_LEAN_AND_MEAN)

add_library(AnshuBioCredentialProvider SHARED
    AnshuBioCredentialProvider.cpp
    AnshuBioCredential.cpp
    Dll.cpp
    AnshuBioCredentialProvider.def
)

target_link_libraries(AnshuBioCredentialProvider PRIVATE
    shlwapi
    advapi32
    user32
    ole32
    oleaut32
    secur32
)

set_target_properties(AnshuBioCredentialProvider PROPERTIES
    OUTPUT_NAME "AnshuBioCredentialProvider"
    PREFIX ""
    SUFFIX ".dll"
)

add_executable(AnshuBioSessionMonitor
    AnshuBioSessionMonitor.cpp
)

target_link_libraries(AnshuBioSessionMonitor PRIVATE
    wtsapi32
    user32
)

set_target_properties(AnshuBioSessionMonitor PROPERTIES
    OUTPUT_NAME "AnshuBioSessionMonitor"
)

