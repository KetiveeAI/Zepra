# =============================================================================
# ZepraScript Tools
# =============================================================================

# REPL - Interactive JavaScript shell
add_executable(zepra-repl
    zepra-repl.cpp
)

target_link_libraries(zepra-repl
    PRIVATE
        zepra-core
)

target_include_directories(zepra-repl
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

# Bytecode Dumper - Debug tool for inspecting bytecode
add_executable(zepra-dump-bytecode
    zepra-dump-bytecode.cpp
)

target_link_libraries(zepra-dump-bytecode
    PRIVATE
        zepra-core
)

target_include_directories(zepra-dump-bytecode
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

# Install tools
install(TARGETS zepra-repl zepra-dump-bytecode
    RUNTIME DESTINATION bin
)
