cmake_minimum_required(VERSION 3.16)
project(nxjson VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Sources
set(NXJSON_SOURCES
    parser.cpp
    writer.cpp
    pointer.cpp
)

# Create library
add_library(nxjson STATIC ${NXJSON_SOURCES})

target_include_directories(nxjson PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Compiler warnings
target_compile_options(nxjson PRIVATE
    -Wall -Wextra -Wpedantic
)

# Install
install(TARGETS nxjson
    ARCHIVE DESTINATION lib
)

install(FILES include/nxjson.h DESTINATION include)
