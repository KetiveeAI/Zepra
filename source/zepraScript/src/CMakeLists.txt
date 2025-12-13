# =============================================================================
# ZepraScript Core Engine Library
# =============================================================================

set(ZEPRA_CORE_SOURCES
    # Frontend
    frontend/lexer.cpp
    frontend/parser.cpp
    frontend/ast.cpp
    frontend/source_code.cpp
    frontend/syntax_checker.cpp
    
    # Runtime
    runtime/value.cpp
    runtime/object.cpp
    runtime/function.cpp
    runtime/vm.cpp
    runtime/environment.cpp
    runtime/global_object.cpp
    runtime/module.cpp
    runtime/module_loader.cpp
    runtime/promise.cpp
    runtime/inline_cache.cpp
    runtime/iterator.cpp
    runtime/proxy.cpp
    
    # Garbage Collector
    gc/heap.cpp
    gc/allocator.cpp
    gc/marking.cpp
    gc/sweeping.cpp
    gc/handle.cpp
    gc/write_barrier.cpp
    gc/incremental_gc.cpp
    
    # Bytecode
    bytecode/bytecode_generator.cpp
    bytecode/opcode.cpp
    bytecode/peephole_optimizer.cpp
    
    # Interpreter
    interpreter/interpreter.cpp
    interpreter/stack_frame.cpp
    
    # Builtins
    builtins/object_builtins.cpp
    builtins/array.cpp
    builtins/string.cpp
    builtins/number.cpp
    builtins/boolean.cpp
    builtins/math.cpp
    builtins/date.cpp
    builtins/console.cpp
    builtins/json.cpp
    builtins/map.cpp
    builtins/set.cpp
    builtins/regexp.cpp
    builtins/symbol.cpp
    builtins/weakmap.cpp
    builtins/typed_array.cpp
    builtins/data_view.cpp
    
    # JIT
    jit/jit_profiler.cpp
    jit/baseline_jit.cpp
    jit/osr.cpp
    
    # Memory
    memory/page_allocator.cpp
    memory/arena_allocator.cpp
    memory/memory_pool.cpp
    
    # API
    api/isolate.cpp
    api/context.cpp
    api/local_handle.cpp
    
    # Debug
    debug/debug_api.cpp
    debug/breakpoint_manager.cpp
    debug/call_stack_info.cpp
    debug/execution_control.cpp
    debug/debugger.cpp
    debug/inspector.cpp
    debug/profiler.cpp
    debug/console.cpp
    
    # Utils
    utils/platform.cpp
    utils/unicode.cpp
    utils/string_builder.cpp
    
    # Host
    host/script_engine.cpp
    host/native_function.cpp
    
    # Browser
    browser/window.cpp
    browser/document.cpp
    browser/fetch.cpp
    browser/url.cpp
    browser/worker.cpp
    browser/event_system.cpp
    browser/storage_api.cpp
    browser/secure_storage.cpp
    browser/password_vault.cpp
)

# Create core library
add_library(zepra-core STATIC ${ZEPRA_CORE_SOURCES})

target_include_directories(zepra-core
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link threading support
find_package(Threads REQUIRED)
find_package(CURL REQUIRED)

target_link_libraries(zepra-core
    PUBLIC
        Threads::Threads
        CURL::libcurl
)

# Platform-specific settings
if(UNIX AND NOT APPLE)
    target_link_libraries(zepra-core PUBLIC dl)
endif()

# Add compile definitions
target_compile_definitions(zepra-core
    PRIVATE
        ZEPRA_VERSION="${PROJECT_VERSION}"
        ZEPRA_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        ZEPRA_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        ZEPRA_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)
