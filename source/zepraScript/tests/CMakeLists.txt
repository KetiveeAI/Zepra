# =============================================================================
# ZepraScript Tests
# =============================================================================

# Find Google Test
find_package(GTest QUIET)

if(NOT GTest_FOUND)
    # Download and build Google Test if not found
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

# =============================================================================
# Unit Tests
# =============================================================================
set(UNIT_TEST_SOURCES
    unit/value_tests.cpp
    unit/lexer_tests.cpp
    unit/parser_tests.cpp
    unit/vm_tests.cpp
    unit/gc_tests.cpp
    unit/builtin_tests.cpp
    unit/url_tests.cpp
)

add_executable(zepra-unit-tests ${UNIT_TEST_SOURCES})

target_link_libraries(zepra-unit-tests
    PRIVATE
        zepra-core
        GTest::gtest
        GTest::gtest_main
)

target_include_directories(zepra-unit-tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

# Register tests with CTest
include(GoogleTest)
gtest_discover_tests(zepra-unit-tests)

# =============================================================================
# Integration Tests
# =============================================================================
set(INTEGRATION_TEST_SOURCES
    integration/module_tests.cpp
    integration/promise_tests.cpp
    integration/fetch_tests.cpp
)

add_executable(zepra-integration-tests ${INTEGRATION_TEST_SOURCES})

target_link_libraries(zepra-integration-tests
    PRIVATE
        zepra-core
        GTest::gtest
        GTest::gtest_main
)

target_include_directories(zepra-integration-tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

gtest_discover_tests(zepra-integration-tests)
