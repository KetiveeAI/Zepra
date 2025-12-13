cmake_minimum_required(VERSION 3.16)
project(ZepraEngine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find SDL2
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
find_package(OpenGL QUIET)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../webCore/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../zepraScript/include
    ${SDL2_INCLUDE_DIRS}
)

# Link directories
link_directories(
    ${SDL2_LIBRARY_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/../webCore/build
    ${CMAKE_CURRENT_SOURCE_DIR}/../zepraScript/build/lib
    ${CMAKE_CURRENT_SOURCE_DIR}/../networking/build
)

# Window demo executable
add_executable(zepra-browser
    examples/zepra_window_demo.cpp
)

target_link_libraries(zepra-browser
    ${SDL2_LIBRARIES}
)

# OpenGL demo executable
add_executable(gl-demo
    examples/gl_demo.cpp
)

target_include_directories(gl-demo PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../webCore/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../zepraScript/include
)

target_link_libraries(gl-demo
    webcore
    zepra-core
    ${SDL2_LIBRARIES}
)

if(OpenGL_FOUND)
    target_link_libraries(gl-demo OpenGL::GL)
    target_compile_definitions(gl-demo PRIVATE ZEPRA_HAS_OPENGL)
endif()

# WebGL 3D demo executable
add_executable(webgl-demo
    examples/webgl_demo.cpp
)

target_include_directories(webgl-demo PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../webCore/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../zepraScript/include
)

target_link_libraries(webgl-demo
    webcore
    zepra-core
    ${SDL2_LIBRARIES}
)

if(OpenGL_FOUND)
    target_link_libraries(webgl-demo OpenGL::GL)
    target_compile_definitions(webgl-demo PRIVATE ZEPRA_HAS_OPENGL)
endif()

# Main Browser Application
add_executable(zepra-browser-app
    src/zepra_browser_app.cpp
)

target_include_directories(zepra-browser-app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../webCore/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../webcore/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../zepraScript/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../networking/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../storage/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../devtools/include
)

target_link_libraries(zepra-browser-app
    webcore
    networking
    zepra-core
    ${SDL2_LIBRARIES}
    SDL2_ttf
    SDL2_image
    curl
    ssl
    crypto
    pthread
)

if(OpenGL_FOUND)
    target_link_libraries(zepra-browser-app OpenGL::GL)
    target_compile_definitions(zepra-browser-app PRIVATE ZEPRA_HAS_OPENGL)
endif()

# Output to bin directory
set_target_properties(zepra-browser gl-demo webgl-demo zepra-browser-app PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../bin
)
