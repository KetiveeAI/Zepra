# WebGPU Library for Zepra Browser

cmake_minimum_required(VERSION 3.16)

set(WEBGPU_SOURCES
    src/webgpu_context.cpp
    src/webgpu_bindings.cpp
)

set(WEBGPU_HEADERS
    include/webgpu/webgpu_context.hpp
    include/webgpu/webgpu_bindings.hpp
)

add_library(webgpu STATIC ${WEBGPU_SOURCES} ${WEBGPU_HEADERS})

target_include_directories(webgpu
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link to zepra-core for runtime types
target_link_libraries(webgpu
    PRIVATE
        zepra-core
)

# Set C++ standard
target_compile_features(webgpu PUBLIC cxx_std_17)
