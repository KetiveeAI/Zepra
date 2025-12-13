# ZepraBrowser - Networking Module
cmake_minimum_required(VERSION 3.20)

project(zepra-networking VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
set(NETWORKING_SOURCES
    src/ssl_context.cpp
    src/cookie_manager.cpp
    src/http_client.cpp
    src/http_request.cpp
    src/http_response.cpp
    src/http_cache.cpp
    src/dns_resolver.cpp
    src/websocket_client.cpp
    src/resource_loader.cpp
)

add_library(networking STATIC ${NETWORKING_SOURCES})

target_include_directories(networking PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Dependencies
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

target_link_libraries(networking 
    PUBLIC 
        OpenSSL::SSL 
        OpenSSL::Crypto
        Threads::Threads
)

# Compiler flags
target_compile_options(networking PRIVATE
    -Wall -Wextra -Wpedantic
)
