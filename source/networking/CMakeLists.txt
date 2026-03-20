# ZepraBrowser - Networking Module
cmake_minimum_required(VERSION 3.20)

project(zepra-networking VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
set(NETWORKING_SOURCES
    ssl_context.cpp
    cookie_manager.cpp
    http_client.cpp
    http_request.cpp
    http_response.cpp
    http_cache.cpp
    dns_resolver.cpp
    websocket_client.cpp
    resource_loader.cpp
)

add_library(networking STATIC ${NETWORKING_SOURCES})

target_include_directories(networking PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Dependencies
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

find_package(CURL REQUIRED)

target_link_libraries(networking 
    PUBLIC 
        OpenSSL::SSL 
        OpenSSL::Crypto
        Threads::Threads
        CURL::libcurl
)

# Compiler flags
target_compile_options(networking PRIVATE
    -Wall -Wextra -Wpedantic
)
