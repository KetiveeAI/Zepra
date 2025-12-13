cmake_minimum_required(VERSION 3.16)
project(zepra-integration LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
set(INTEGRATION_SOURCES
    src/dom_bridge.cpp
    src/browser_context.cpp
)

# Header files
set(INTEGRATION_HEADERS
    include/integration/dom_bridge.hpp
    include/integration/browser_context.hpp
)

# Library
add_library(zepra-integration STATIC ${INTEGRATION_SOURCES})

target_include_directories(zepra-integration PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/source/zepraScript/include
    ${CMAKE_SOURCE_DIR}/source/webCore/include
)

# Link dependencies
target_link_libraries(zepra-integration
    zepra-core
    webcore
)

# Install
install(TARGETS zepra-integration
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/ DESTINATION include)
