cmake_minimum_required(VERSION 3.20)
project(ZepraScript
    VERSION 1.0.0
    DESCRIPTION "ZebraScript JavaScript Engine for ZepraBrowser"
    LANGUAGES CXX
)

# =============================================================================
# CMake Configuration
# =============================================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Module path for custom CMake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# =============================================================================
# Build Options
# =============================================================================
option(ZEPRA_BUILD_DEVTOOLS "Build Zepra DevTools UI" OFF)
option(ZEPRA_BUILD_CDP_EXTENSION "Build CDP extension (for 3rd party tools)" OFF)
option(ZEPRA_BUILD_TESTS "Build unit and integration tests" ON)
option(ZEPRA_BUILD_BENCHMARKS "Build performance benchmarks" OFF)
option(ZEPRA_BUILD_TOOLS "Build CLI tools (REPL, etc.)" ON)
option(ZEPRA_ENABLE_ASAN "Enable Address Sanitizer" OFF)
option(ZEPRA_ENABLE_UBSAN "Enable Undefined Behavior Sanitizer" OFF)
option(ZEPRA_ENABLE_TSAN "Enable Thread Sanitizer" OFF)

# =============================================================================
# Compiler Warnings
# =============================================================================
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Werror=return-type
        -Wno-unused-parameter
        -fno-omit-frame-pointer
    )
    
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g -O0)
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-O3 -DNDEBUG)
    elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        add_compile_options(-O2 -g)
    endif()
endif()

if(MSVC)
    add_compile_options(/W4 /WX /permissive-)
endif()

# =============================================================================
# Sanitizers
# =============================================================================
if(ZEPRA_ENABLE_ASAN)
    add_compile_options(-fsanitize=address)
    add_link_options(-fsanitize=address)
endif()

if(ZEPRA_ENABLE_UBSAN)
    add_compile_options(-fsanitize=undefined)
    add_link_options(-fsanitize=undefined)
endif()

if(ZEPRA_ENABLE_TSAN)
    add_compile_options(-fsanitize=thread)
    add_link_options(-fsanitize=thread)
endif()

# =============================================================================
# Output Directories
# =============================================================================
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# =============================================================================
# Include Directories - Core Headers
# =============================================================================
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# =============================================================================
# Core Engine Library (always built)
# =============================================================================
add_subdirectory(src)

# =============================================================================
# Optional: Zepra DevTools (Primary Debugging Tool)
# =============================================================================
if(ZEPRA_BUILD_DEVTOOLS)
    find_package(Qt6 COMPONENTS Widgets QUIET)
    if(Qt6_FOUND)
        add_subdirectory(zepra-devtools)
    else()
        message(WARNING "Qt6 not found, skipping DevTools build")
    endif()
endif()

# =============================================================================
# Optional: CDP Extension (Chrome DevTools Compatibility)
# =============================================================================
if(ZEPRA_BUILD_CDP_EXTENSION)
    add_subdirectory(cdp-extension)
endif()

# =============================================================================
# CLI Tools (REPL, bytecode dumper, etc.)
# =============================================================================
if(ZEPRA_BUILD_TOOLS)
    add_subdirectory(tools)
endif()

# =============================================================================
# Tests
# =============================================================================
if(ZEPRA_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# =============================================================================
# Benchmarks
# =============================================================================
if(ZEPRA_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

# =============================================================================
# Installation
# =============================================================================
install(DIRECTORY include/zeprascript
    DESTINATION include
    FILES_MATCHING PATTERN "*.hpp"
)

install(TARGETS zepra-core
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

# =============================================================================
# Summary
# =============================================================================
message(STATUS "")
message(STATUS "=== ZepraScript Build Configuration ===")
message(STATUS "Version:        ${PROJECT_VERSION}")
message(STATUS "Build type:     ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Compiler:   ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "")
message(STATUS "Options:")
message(STATUS "  DevTools:     ${ZEPRA_BUILD_DEVTOOLS}")
message(STATUS "  CDP Extension: ${ZEPRA_BUILD_CDP_EXTENSION}")
message(STATUS "  Tests:        ${ZEPRA_BUILD_TESTS}")
message(STATUS "  Benchmarks:   ${ZEPRA_BUILD_BENCHMARKS}")
message(STATUS "  Tools:        ${ZEPRA_BUILD_TOOLS}")
message(STATUS "  ASAN:         ${ZEPRA_ENABLE_ASAN}")
message(STATUS "  UBSAN:        ${ZEPRA_ENABLE_UBSAN}")
message(STATUS "  TSAN:         ${ZEPRA_ENABLE_TSAN}")
message(STATUS "========================================")
message(STATUS "")
