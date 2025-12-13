cmake_minimum_required(VERSION 3.16)
project(Zepra VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build options
option(USE_UNIFIED_SYSTEM "Use unified Zepra Core system" OFF)
option(USE_NXRENDER "Use NXRENDER native rendering (no SDL)" OFF)

# Set build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Platform-specific settings
if(WIN32)
    # Windows-specific settings
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    
    # Try to find SDL2 using different methods
    find_package(SDL2 QUIET)
    if(NOT SDL2_FOUND)
        # Try to find SDL2 using pkg-config (if available)
        find_package(PkgConfig QUIET)
        if(PkgConfig_FOUND)
            pkg_check_modules(SDL2 QUIET sdl2)
        endif()
    endif()
    
    # Try to find CURL
    find_package(CURL QUIET)
    if(NOT CURL_FOUND)
        # Try to find CURL using pkg-config
        if(PkgConfig_FOUND)
            pkg_check_modules(CURL QUIET libcurl)
        endif()
    endif()
    
    # Try to find OpenSSL
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(OPENSSL REQUIRED openssl)
    else()
        find_package(OpenSSL REQUIRED)
    endif()
    if(NOT OPENSSL_FOUND)
        # If still not found, try to find it manually
        find_path(OPENSSL_INCLUDE_DIR openssl/ssl.h)
        find_library(OPENSSL_CRYPTO_LIBRARY crypto)
        find_library(OPENSSL_SSL_LIBRARY ssl)
        if(OPENSSL_INCLUDE_DIR AND OPENSSL_CRYPTO_LIBRARY AND OPENSSL_SSL_LIBRARY)
            set(OPENSSL_FOUND TRUE)
            set(OPENSSL_INCLUDE_DIRS ${OPENSSL_INCLUDE_DIR})
            set(OPENSSL_LIBRARIES ${OPENSSL_SSL_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY})
        endif()
    endif()
    
    # Link Windows libraries
    set(PLATFORM_LIBS ws2_32 wininet)
    
elseif(APPLE)
    # macOS-specific settings
    find_package(SDL2 REQUIRED)
    find_package(CURL REQUIRED)
    set(PLATFORM_LIBS "")
    
else()
    # Linux-specific settings
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED sdl2)
    pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
    pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)
    pkg_check_modules(CURL REQUIRED libcurl)
    pkg_check_modules(OPENSSL REQUIRED openssl)
    set(PLATFORM_LIBS "")
endif()

# Find OpenGL
find_package(OpenGL REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/external)
include_directories(${CMAKE_SOURCE_DIR}/binof\ kit+/include)
include_directories(${CMAKE_SOURCE_DIR}/binof\ kit+/json)

# ============================================================================
# Engine subdirectories (zepraScript, webCore, etc.)
# ============================================================================
# Build ZepraScript JS engine (creates zepra-core library)
add_subdirectory(source/zepraScript)
include_directories(${CMAKE_SOURCE_DIR}/source/zepraScript/include)

# Build WebCore rendering engine (creates webcore library)
add_subdirectory(source/webCore)
include_directories(${CMAKE_SOURCE_DIR}/source/webCore/include)

# Build Networking module
add_subdirectory(source/networking)
include_directories(${CMAKE_SOURCE_DIR}/source/networking/include)

# Build ZepraEngine demo applications
add_subdirectory(source/zepraEngine)

# Build Integration layer (JS↔DOM bridge)
add_subdirectory(source/integration)
include_directories(${CMAKE_SOURCE_DIR}/source/integration/include)

# Source files
if(USE_NXRENDER)
    # NXRENDER native build - no SDL
    set(SOURCES
        src/main_nxrender.cpp
        src/engine/dev_tools.cpp
        src/engine/webkit_engine_stub.cpp
        src/engine/html_parser_stub.cpp
        src/engine/audio_stub.cpp
        src/engine/avmedia_stub.cpp
        src/search/ketivee_search.cpp
        src/ui/window.cpp
        src/ui/tab_manager.cpp
        src/ui/dev_tools_ui.cpp
        src/auth/zepra_auth.cpp
        src/ui/auth_ui.cpp
        src/ui/settings_ui.cpp
        src/sandbox/sandbox_manager.cpp
        src/config/config_manager.cpp
        src/remote/zep_remote.cpp
    )
    set(EXECUTABLE_NAME "zepra_nxrender")
    set(NXRENDER_LIB_PATH "${CMAKE_SOURCE_DIR}/source/nxrender/target/release")
elseif(USE_UNIFIED_SYSTEM)
    set(SOURCES
        src/main_unified.cpp
        src/core/zepra_core.cpp
        src/core/ui_elements.cpp
        src/engine/dev_tools.cpp
        src/engine/download_manager.cpp
        src/search/ketivee_search.cpp
        src/ui/window.cpp
        src/ui/tab_manager.cpp
        src/ui/dev_tools_ui.cpp
        src/auth/zepra_auth.cpp
        src/ui/auth_ui.cpp
        src/ui/settings_ui.cpp
        src/sandbox/sandbox_manager.cpp
        src/config/config_manager.cpp
        src/platform/platform_infrastructure.cpp
    )
    set(EXECUTABLE_NAME "zepra_core_browser")
else()
    set(SOURCES
        src/main.cpp
        src/engine/dev_tools.cpp
        src/engine/webkit_engine_stub.cpp
        src/engine/audio_stub.cpp
        src/engine/avmedia_stub.cpp
        src/engine/html_parser_stub.cpp
        src/search/ketivee_search.cpp
        src/ui/window.cpp
        src/ui/tab_manager.cpp
        src/ui/dev_tools_ui.cpp
        src/auth/zepra_auth.cpp
        src/ui/auth_ui.cpp
        src/ui/settings_ui.cpp
        src/sandbox/sandbox_manager.cpp
        src/config/config_manager.cpp
    )
    set(EXECUTABLE_NAME "zepra_browser")
endif()

# Ensure SOURCES is defined
if(NOT DEFINED SOURCES)
    message(FATAL_ERROR "SOURCES variable is not defined")
endif()

# Header files
set(HEADERS
    include/common/types.h
    include/common/constants.h
    include/core/zepra_core.h
    include/engine/html_parser.h
    include/engine/webkit_engine.h
    include/engine/dev_tools.h
    include/engine/download_manager.h
    include/search/ketivee_search.h
    include/ui/window.h
    include/ui/tab_manager.h
    include/ui/dev_tools_ui.h
    include/auth/zepra_auth.h
    include/ui/auth_ui.h
    include/ui/settings_ui.h
    include/net/http.h
    include/sandbox/sandbox_manager.h
    include/config/dual_config.h
    src/config/config_manager.h
)

# Create main executable
add_executable(${EXECUTABLE_NAME} ${SOURCES} ${HEADERS})

# Link libraries based on build mode
if(USE_NXRENDER)
    # NXRENDER mode - link nxrender_ffi, X11, OpenGL, FreeType
    find_package(X11 REQUIRED)
    find_package(Freetype REQUIRED)
    target_link_libraries(${EXECUTABLE_NAME} 
        ${NXRENDER_LIB_PATH}/libnxrender_ffi.so
        ${X11_LIBRARIES}
        GL GLX
        pthread dl m
        Freetype::Freetype
        webcore
        zepra-integration
        networking
    )
    target_include_directories(${EXECUTABLE_NAME} PRIVATE 
        ${CMAKE_SOURCE_DIR}/include
        ${X11_INCLUDE_DIR}
        ${FREETYPE_INCLUDE_DIRS}
    )
    message(STATUS "Using NXRENDER native rendering with X11/OpenGL/FreeType")
else()
    # SDL mode
    if(SDL2_FOUND)
        if(SDL2_LIBRARIES)
            target_link_libraries(${EXECUTABLE_NAME} ${SDL2_LIBRARIES})
            target_include_directories(${EXECUTABLE_NAME} PRIVATE ${SDL2_INCLUDE_DIRS})
        else()
            target_link_libraries(${EXECUTABLE_NAME} SDL2::SDL2)
        endif()
    else()
        message(FATAL_ERROR "SDL2 not found. Use -DUSE_NXRENDER=ON for native rendering.")
    endif()
endif()

if(SDL2_IMAGE_FOUND)
    target_link_libraries(${EXECUTABLE_NAME} ${SDL2_IMAGE_LIBRARIES})
    target_include_directories(${EXECUTABLE_NAME} PRIVATE ${SDL2_IMAGE_INCLUDE_DIRS})
else()
     # Fallback or error - for now optional but recommended
     message(STATUS "SDL2_image not found via pkg-config")
endif()

if(SDL2_TTF_FOUND)
    target_link_libraries(${EXECUTABLE_NAME} ${SDL2_TTF_LIBRARIES})
    target_include_directories(${EXECUTABLE_NAME} PRIVATE ${SDL2_TTF_INCLUDE_DIRS})
else()
     message(STATUS "SDL2_ttf not found via pkg-config")
endif()

if(CURL_FOUND)
    if(CURL_LIBRARIES)
        target_link_libraries(${EXECUTABLE_NAME} ${CURL_LIBRARIES})
        target_include_directories(${EXECUTABLE_NAME} PRIVATE ${CURL_INCLUDE_DIRS})
    else()
        target_link_libraries(${EXECUTABLE_NAME} CURL::libcurl)
    endif()
else()
    message(FATAL_ERROR "CURL not found. Please install libcurl development libraries.")
endif()

if(OPENSSL_FOUND)
    if(OPENSSL_LIBRARIES)
        target_link_libraries(${EXECUTABLE_NAME} ${OPENSSL_LIBRARIES})
        target_include_directories(${EXECUTABLE_NAME} PRIVATE ${OPENSSL_INCLUDE_DIRS})
    else()
        target_link_libraries(${EXECUTABLE_NAME} OpenSSL::SSL OpenSSL::Crypto)
    endif()
else()
    message(FATAL_ERROR "OpenSSL not found. Please install OpenSSL development libraries.")
endif()

# nlohmann/json.hpp is header-only, no linking needed
# The include path is already set up in include_directories

# Link json-c library (used by zepra_auth)
find_package(PkgConfig QUIET)
if(PKGCONFIG_FOUND)
    pkg_check_modules(JSON_C json-c)
endif()
if(JSON_C_FOUND)
    target_link_libraries(${EXECUTABLE_NAME} ${JSON_C_LIBRARIES})
    target_include_directories(${EXECUTABLE_NAME} PRIVATE ${JSON_C_INCLUDE_DIRS})
else()
    # Fallback to direct linking
    target_link_libraries(${EXECUTABLE_NAME} json-c)
endif()

# Link OpenGL and platform libraries
target_link_libraries(${EXECUTABLE_NAME} ${OPENGL_LIBRARIES} ${PLATFORM_LIBS})

# Link engine libraries (zepraScript, webCore, integration)
target_link_libraries(${EXECUTABLE_NAME} zepra-core webcore zepra-integration)

# Compiler flags
target_compile_options(${EXECUTABLE_NAME} PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /utf-8>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

# Test executable
add_executable(zepra_test test/test_basic.cpp ${HEADERS})

# Configuration test executable
add_executable(config_test src/config/config_test.cpp src/config/config_manager.cpp)

# Link test executable with same libraries
if(SDL2_FOUND)
    if(SDL2_LIBRARIES)
        target_link_libraries(zepra_test ${SDL2_LIBRARIES})
        target_include_directories(zepra_test PRIVATE ${SDL2_INCLUDE_DIRS})
    else()
        target_link_libraries(zepra_test SDL2::SDL2)
    endif()
endif()

if(CURL_FOUND)
    if(CURL_LIBRARIES)
        target_link_libraries(zepra_test ${CURL_LIBRARIES})
        target_include_directories(zepra_test PRIVATE ${CURL_INCLUDE_DIRS})
    else()
        target_link_libraries(zepra_test CURL::libcurl)
    endif()
endif()

target_link_libraries(zepra_test ${OPENGL_LIBRARIES} ${PLATFORM_LIBS})

target_compile_options(zepra_test PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /utf-8>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

# Link configuration test executable
target_include_directories(config_test PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_options(config_test PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /utf-8>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

# Add test
enable_testing()
add_test(NAME ZepraBasicTest COMMAND zepra_test)

# Installation rules
install(TARGETS ${EXECUTABLE_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(FILES ${HEADERS}
    DESTINATION include/zepra
)

# Package configuration
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/ZepraConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

# Documentation
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
    "${CMAKE_CURRENT_BINARY_DIR}/README.md"
    COPYONLY
)

# Print configuration summary
message(STATUS "Zepra Browser Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  SDL2 found: ${SDL2_FOUND}")
message(STATUS "  OpenGL found: ${OPENGL_FOUND}")
message(STATUS "  CURL found: ${CURL_FOUND}")
message(STATUS "  Platform: ${CMAKE_SYSTEM_NAME}")

target_include_directories(${EXECUTABLE_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external
)