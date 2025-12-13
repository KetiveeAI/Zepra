# Zepra DevTools - Native Developer Tools for ZepraBrowser
cmake_minimum_required(VERSION 3.16)
project(zepra-devtools VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find SDL2
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)

# DevTools Sources - standalone window only
set(DEVTOOLS_SOURCES
    src/devtools_window.cpp
)

# DevTools static library
add_library(devtools STATIC ${DEVTOOLS_SOURCES})

target_include_directories(devtools PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_TTF_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/../zepraScript/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../webCore/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../webcore/include
)

target_link_libraries(devtools PRIVATE
    ${SDL2_LIBRARIES}
    ${SDL2_TTF_LIBRARIES}
)

# Link to ZepraScript engine if available
if(TARGET zepra-core)
    target_link_libraries(devtools PRIVATE zepra-core)
    target_compile_definitions(devtools PRIVATE ZEPRA_VM_AVAILABLE)
endif()

set_target_properties(devtools PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)
