cmake_minimum_required(VERSION 3.16)
project(nxhttp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Sources
set(NXHTTP_SOURCES
    url.cpp
    headers.cpp
    client.cpp
    connection_pool.cpp
    cache.cpp
    punycode.cpp
    http2.cpp
    file_quarantine.cpp
)

# Create library
add_library(nxhttp STATIC ${NXHTTP_SOURCES})

target_include_directories(nxhttp PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../networking/include  # For SSL context
    ${CMAKE_CURRENT_SOURCE_DIR}/../networking  # ssl_context.hpp lives here
)

# Platform-specific socket libraries
if(WIN32)
    target_link_libraries(nxhttp ws2_32)
endif()

# Link networking library for SSL/TLS support
# When used as part of the browser, networking library provides OpenSSL wrapper
if(TARGET networking)
    target_link_libraries(nxhttp PRIVATE networking)
else()
    # Find OpenSSL directly if networking not available
    find_package(OpenSSL)
    if(OpenSSL_FOUND)
        target_link_libraries(nxhttp PRIVATE OpenSSL::SSL OpenSSL::Crypto)
        target_compile_definitions(nxhttp PRIVATE NXHTTP_HAS_OPENSSL=1)
    endif()
endif()

# Compiler warnings
target_compile_options(nxhttp PRIVATE
    -Wall -Wextra -Wpedantic
)

# Install
install(TARGETS nxhttp
    ARCHIVE DESTINATION lib
)

install(FILES include/nxhttp.h DESTINATION include)

