# CMakeList for HttpNetworkTransport tests module
#
# © 2024 by Hatem Nabli


cmake_minimum_required(VERSION 3.8)

set(this HttpNetworkTransportTests)

set(SOURCES
    src/HttpServerNetworkTransportTests.cpp
    src/HttpClientNetworkTransportTests.cpp
)

add_executable(${this} ${SOURCES})
set_target_properties(${this} PROPERTIES
    FOLDER Tests
)

target_include_directories(${this} PRIVATE ..)

target_link_libraries(${this}
    gtest_main
    Http
    HttpNetworkTransport
    SystemUtils
)

add_test(
    NAME ${this}
    COMMAND ${this}
)