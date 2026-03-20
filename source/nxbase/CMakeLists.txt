cmake_minimum_required(VERSION 3.16)
project(nxbase VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Sources
set(NXBASE_SOURCES
    buffer.cpp
    string.cpp
    allocator.cpp
    log.cpp
    hashmap.cpp
    timer.cpp
    file_io.cpp
    arena.cpp
)

# Create library
add_library(nxbase STATIC ${NXBASE_SOURCES})

target_include_directories(nxbase PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Compiler warnings
target_compile_options(nxbase PRIVATE
    -Wall -Wextra -Wpedantic
)

# Install
install(TARGETS nxbase
    ARCHIVE DESTINATION lib
)

install(FILES include/nxbase.h DESTINATION include)
