cmake_minimum_required(VERSION 3.16)
project(AnshuBioTests LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(AnshuBioTests
    TestRunner.cpp
    ${CORE_SOURCES}
)

target_include_directories(AnshuBioTests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
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
