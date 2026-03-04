cmake_minimum_required(VERSION 3.16)
project(Zepra VERSION 1.0.0 LANGUAGES CXX)

# ============================================================================
# Build System - Prefer Ninja for faster, lighter builds
# ============================================================================
# To use Ninja: cmake -G Ninja -B build
# Or set CMAKE_GENERATOR=Ninja environment variable
# ============================================================================

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Performance optimizations for build
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # For IDE support

# Limit parallel jobs to prevent system overload (adjust based on your system)
if(NOT DEFINED CMAKE_BUILD_PARALLEL_LEVEL)
    # Use half of available cores to prevent overload
    cmake_host_system_information(RESULT NPROC QUERY NUMBER_OF_LOGICAL_CORES)
    math(EXPR PARALLEL_JOBS "${NPROC} / 2")
    if(PARALLEL_JOBS LESS 1)
        set(PARALLEL_JOBS 1)
    endif()
    message(STATUS "Build parallelism: ${PARALLEL_JOBS} jobs (of ${NPROC} cores)")
endif()

# Ninja-specific optimizations
if(CMAKE_GENERATOR MATCHES "Ninja")
    message(STATUS "Using Ninja generator - optimized for performance")
    # Ninja pool for linking (reduce memory usage)
    set_property(GLOBAL PROPERTY JOB_POOLS link_pool=2)
endif()

# Build options
option(USE_UNIFIED_SYSTEM "Use unified Zepra Core system" OFF)
option(USE_NXRENDER "Use NXRENDER native rendering (no SDL)" OFF)
option(USE_ZEPRA_STACK "Use custom Zepra libraries (NxHTTP, NxJSON, NxCrypto)" OFF)

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

    set(PLATFORM_LIBS "")
    
else()
    # Linux-specific settings
    find_package(PkgConfig REQUIRED)
    find_package(X11 REQUIRED)
    find_package(Freetype REQUIRED)
    
    pkg_check_modules(SDL2 REQUIRED sdl2)
    pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
    pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)

    pkg_check_modules(OPENSSL REQUIRED openssl)
    set(PLATFORM_LIBS ${X11_LIBRARIES} Freetype::Freetype)
endif()

# Find OpenGL
find_package(OpenGL REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/external)

# ============================================================================
# Engine subdirectories (zepraScript, webCore, etc.)
# ============================================================================
# Build ZepraScript JS engine (creates zepra-core library)
add_subdirectory(source/zepraScript)
include_directories(${CMAKE_SOURCE_DIR}/source/zepraScript)

# Build WebCore rendering engine (creates webcore library)
add_subdirectory(source/webCore)
include_directories(${CMAKE_SOURCE_DIR}/source/webCore/include)

# Build Networking module
add_subdirectory(source/networking)
include_directories(${CMAKE_SOURCE_DIR}/source/networking/include)

# Build NXRender-cpp graphics library
add_subdirectory(source/nxrender-cpp)

# NOTE: zepraEngine demos moved to test/demos/zepraEngine/ (SDL removed)

# Build Integration layer (JS↔DOM bridge)
add_subdirectory(source/integration)
include_directories(${CMAKE_SOURCE_DIR}/source/integration/include)

# Build NXAudio (Audio Engine)
add_subdirectory(src/nxaudio)
include_directories(${CMAKE_SOURCE_DIR}/src/nxaudio/include)

# ============================================================================
# Custom Zepra Stack (optional - replace system libs)
# ============================================================================
# Custom Zepra Stack (Unconditional)
# ============================================================================
    message(STATUS "Using custom Zepra Stack libraries")
    add_subdirectory(source/nxbase)
    add_subdirectory(source/nxjson)
    add_subdirectory(source/nxhttp)
    add_subdirectory(source/nxxml)
    add_subdirectory(source/nxcrypto)
    
    include_directories(${CMAKE_SOURCE_DIR}/source/nxbase/include)
    include_directories(${CMAKE_SOURCE_DIR}/source/nxjson/include)
    include_directories(${CMAKE_SOURCE_DIR}/source/nxhttp/include)
    include_directories(${CMAKE_SOURCE_DIR}/source/nxxml/include)
    include_directories(${CMAKE_SOURCE_DIR}/source/nxcrypto/include)

# Source files
if(USE_NXRENDER)
    # NXRENDER native build - no SDL
    set(SOURCES
        src/main.cpp
        src/zepra_browser.cpp
        src/engine/dev_tools.cpp
        src/engine/webkit_engine_stub.cpp
        src/engine/html_parser_stub.cpp
        src/engine/audio_stub.cpp
        src/engine/avmedia_stub.cpp
        src/engine/console_log.cpp
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
        src/remote/zep_remote.cpp
        src/network/network_monitor.cpp
        src/network/monitored_http.cpp
        src/security/security_checker.cpp
        src/browser/mouse_handler.cpp
        src/browser/layout_engine.cpp
        src/browser/css_utils.cpp
        # New modular architecture
        src/storage/bookmark_manager.cpp
        src/storage/history_database.cpp
        src/storage/settings_store.cpp
        src/browser/navigation_controller.cpp
        src/browser/rewards_manager.cpp
        src/ai/ai_sidebar.cpp
        # Memory optimization
        src/memory/memory_pools.cpp
        # src/browser/tab_suspender.cpp  # DISABLED: ZepraBrowser::Tab vs zepra::Tab mismatch
        src/browser/lazy_image_loader.cpp
        # WebCore integration
        src/webcore_integration.cpp
        # NXRender-Layout bridge
        src/browser/nxrender_layout_bridge.cpp
        # Stubs for missing WebCore symbols (WebGL, etc.)
        src/stubs/webgl_stubs.cpp
        src/stubs/tab_suspender_stubs.cpp
    )
    set(EXECUTABLE_NAME "zepra_browser")
    # NXRender-cpp is built as subdirectory, no external lib path needed
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
        src/zepra_browser.cpp
        src/engine/dev_tools.cpp
        src/engine/webkit_engine_stub.cpp
        src/engine/audio_stub.cpp
        src/engine/avmedia_stub.cpp
        src/engine/html_parser_stub.cpp
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
        src/network/network_monitor.cpp
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
    # CURL removed - now using nxhttp
    find_package(PkgConfig REQUIRED)
    # Note: Using nxjson instead of jsoncpp (line 266)
    
    target_link_libraries(${EXECUTABLE_NAME} 
        webcore
        nxrender
        ${X11_LIBRARIES}
        X11
        GL GLX
        pthread dl m z
        Freetype::Freetype
        zepra-integration
        networking
        nxaudio_static
        nxhttp
        nxbase
        nxcrypto
        nxjson
        nxxml
    )
    target_include_directories(${EXECUTABLE_NAME} PRIVATE 
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/include/storage
        ${CMAKE_SOURCE_DIR}/include/browser
        ${CMAKE_SOURCE_DIR}/include/input
        ${CMAKE_SOURCE_DIR}/include/ai
        ${CMAKE_SOURCE_DIR}/source/nxrender-cpp/include
        ${X11_INCLUDE_DIR}
        ${FREETYPE_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/../../REOX/reolab/runtime
    )
    target_compile_definitions(${EXECUTABLE_NAME} PRIVATE USE_WEBCORE)
    message(STATUS "Using NXRENDER + REOX native rendering with X11/OpenGL/FreeType (NO SDL)")
else()
    # SDL mode
    if(SDL2_FOUND)
        if(SDL2_LIBRARIES)
            target_link_libraries(${EXECUTABLE_NAME} ${SDL2_LIBRARIES})
            target_include_directories(${EXECUTABLE_NAME} PRIVATE ${SDL2_INCLUDE_DIRS})
        else()
            target_link_libraries(${EXECUTABLE_NAME} SDL2::SDL2)
        endif()
        # USE_WEBCORE needed for gfx namespace in zepra_browser.cpp
        target_compile_definitions(${EXECUTABLE_NAME} PRIVATE USE_WEBCORE)
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
# ketivee_search.cpp now uses nxhttp (no more libcurl)
target_link_libraries(${EXECUTABLE_NAME} 
    zepra-core 
    webcore 
    zepra-integration
    nxhttp
    nxbase
    nxcrypto
    nxjson
    nxxml
)

# Compiler flags
target_compile_options(${EXECUTABLE_NAME} PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /utf-8>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

# Test executable


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
message(STATUS "  HTTP Client: nxhttp (libcurl removed)")
message(STATUS "  Platform: ${CMAKE_SYSTEM_NAME}")



# Tests
add_subdirectory(test)