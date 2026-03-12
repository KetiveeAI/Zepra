# ZepraBrowser Core - Browser infrastructure
cmake_minimum_required(VERSION 3.16)
project(browser_core VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Source files
set(BROWSER_CORE_SOURCES
    task_manager.c
    auto_manager.c
    task_manager_ui.c
)

# Headers
set(BROWSER_CORE_HEADERS
    task_manager.h
    task_manager_cpp.hpp
    auto_manager.h
    task_manager_ui.h
)

# Static library
add_library(browser-core-static STATIC ${BROWSER_CORE_SOURCES})
target_include_directories(browser-core-static PUBLIC include)
set_target_properties(browser-core-static PROPERTIES
    OUTPUT_NAME browser-core
    POSITION_INDEPENDENT_CODE ON
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(browser-core-static PRIVATE pthread)
endif()

# Shared library
add_library(browser-core-shared SHARED ${BROWSER_CORE_SOURCES})
target_include_directories(browser-core-shared PUBLIC include)
set_target_properties(browser-core-shared PROPERTIES
    OUTPUT_NAME browser-core
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(browser-core-shared PRIVATE pthread)
endif()

# Alias
add_library(browser::core ALIAS browser-core-static)

# Install
install(TARGETS browser-core-static browser-core-shared
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(FILES ${BROWSER_CORE_HEADERS}
    DESTINATION browser_core
)
