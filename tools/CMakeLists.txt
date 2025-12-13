cmake_minimum_required(VERSION 3.16)
project(ZepraDemo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find SDL2 and SDL2_ttf
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)

# Include ZepraScript and WebCore
set(ZEPRA_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)
include_directories(${ZEPRA_ROOT}/source/zepraScript/include)
include_directories(${ZEPRA_ROOT}/source/webCore/include)
include_directories(${ZEPRA_ROOT}/source/networking/include)

# Find CURL
find_package(CURL REQUIRED)

# Build demo
add_executable(zepra-demo zepra_demo.cpp)

# Link libraries
target_link_libraries(zepra-demo 
    ${CMAKE_CURRENT_SOURCE_DIR}/../source/webCore/build/libwebcore.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../source/networking/build/libnetworking.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../source/zepraScript/build/lib/libzepra-core.a
    ${SDL2_LIBRARIES}
    ${SDL2_TTF_LIBRARIES}
    CURL::libcurl
)

target_include_directories(zepra-demo PRIVATE 
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_TTF_INCLUDE_DIRS}
)
