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

    # Runtime — Objects
    runtime/objects/value.cpp
    runtime/objects/object.cpp
    runtime/objects/function.cpp
    runtime/objects/symbol.cpp
    runtime/objects/proxy.cpp
    runtime/objects/class_fields.cpp
    runtime/objects/map.cpp
    runtime/objects/set.cpp

    # Runtime — Execution
    runtime/execution/vm.cpp
    runtime/execution/crash_handler.cpp
    runtime/execution/environment.cpp
    runtime/execution/global_object.cpp
    runtime/gc_object_visitor.cpp
    runtime/gc_runtime_bridge.cpp
    runtime/gc_write_barrier.cpp
    runtime/gc_lifecycle_hooks.cpp
    runtime/gc_safepoint_hooks.cpp
    runtime/gc_stats_api.cpp
    runtime/gc_pressure_api.cpp
    runtime/gc_config_api.cpp
    runtime/gc_event_log.cpp
    runtime/gc_vm_allocator.cpp
    runtime/gc_vm_hooks.cpp

    # Runtime — Async
    runtime/async/promise.cpp
    runtime/async/async.cpp
    runtime/async/async_function.cpp
    runtime/async/iterator.cpp

    # Runtime — Handles
    runtime/handles/module.cpp
    runtime/handles/module_loader.cpp
    runtime/handles/inline_cache.cpp

    # Heap (was gc/)
    heap/heap.cpp
    heap/gc_heap.cpp
    heap/concurrent_gc.cpp
    heap/allocator.cpp
    heap/marking.cpp
    heap/sweeping.cpp
    heap/handle.cpp
    heap/write_barrier.cpp
    heap/incremental_gc.cpp
    heap/compacting.cpp
    heap/generational_gc.cpp
    heap/finalizer.cpp
    heap/concurrent_sweeper.cpp
    heap/gc_pipeline.cpp
    heap/heap_controller.cpp
    heap/concurrent_marker.cpp
    heap/gc_allocator.cpp
    heap/scavenger_impl.cpp
    heap/incremental_compactor.cpp
    heap/gc_scheduler_impl.cpp
    heap/remembered_set_impl.cpp
    heap/gc_metrics.cpp
    heap/heap_verifier_impl.cpp
    heap/gc_integration.cpp
    heap/thread_manager.cpp
    heap/gc_handshake.cpp
    heap/parallel_scanner.cpp
    heap/generational_barrier.cpp
    heap/heap_compactor.cpp
    heap/memory_pressure.cpp
    heap/gc_debug.cpp
    heap/gc_unit_tests.cpp
    heap/gc_stress.cpp
    heap/gc_benchmark.cpp
    heap/weak_processing.cpp
    heap/finalizer_engine.cpp
    heap/ephemeron_table.cpp
    heap/jit_gc_bridge.cpp
    heap/code_space_manager.cpp
    heap/deoptimization_engine.cpp
    heap/heap_region.cpp
    heap/slab_allocator.cpp
    heap/concurrent_compactor.cpp
    heap/gc_pipeline_orchestrator.cpp
    heap/heap_controller_v2.cpp
    heap/region_allocator.cpp
    heap/shape_table.cpp
    heap/property_storage.cpp
    heap/object_layout.cpp
    heap/gc_safepoint.cpp
    heap/heap_snapshot.cpp
    heap/gc_tracing_engine.cpp
    heap/gc_heap_facade.cpp
    heap/gc_handle_scope.cpp
    heap/gc_integration_test.cpp
    heap/remembered_set.cpp
    heap/concurrent_marker.cpp
    heap/gc_verifier.cpp
    heap/gc_nursery.cpp
    heap/gc_old_space.cpp
    heap/gc_large_object.cpp
    heap/gc_sweeper.cpp
    heap/gc_scheduler.cpp
    heap/gc_telemetry.cpp
    heap/gc_weak_processor.cpp
    heap/gc_finalizer_queue.cpp
    heap/gc_incremental.cpp
    heap/gc_tlab.cpp
    heap/gc_heap_snapshot_serializer.cpp
    heap/gc_barrier_codegen.cpp
    heap/gc_region_metadata.cpp
    heap/gc_evacuation.cpp
    heap/gc_pretenuring.cpp
    heap/gc_marking_bitmap.cpp
    heap/gc_stack_scanner.cpp
    heap/gc_card_table.cpp
    heap/gc_object_pinning.cpp
    heap/gc_age_table.cpp
    heap/gc_heap_iterator.cpp
    heap/gc_sweep_worklist.cpp
    heap/gc_mark_worklist.cpp
    heap/gc_free_list.cpp
    heap/gc_root_set.cpp
    heap/gc_barrier_verifier.cpp
    heap/gc_object_header.cpp
    heap/gc_segment_manager.cpp
    heap/gc_wb_buffer.cpp
    heap/gc_compact_coordinator.cpp
    heap/gc_promotion_bridge.cpp
    heap/gc_controller.cpp
    heap/gc_heap_init.cpp
    heap/gc_scavenger_controller.cpp
    heap/gc_allocation_tracker.cpp
    heap/gc_concurrent_mark_controller.cpp
    heap/gc_state_machine.cpp
    heap/gc_heap_sizing.cpp
    heap/gc_concurrent_sweep_controller.cpp
    heap/gc_nursery_sizing.cpp
    heap/gc_handle_table.cpp
    heap/gc_marking_visitor.cpp
    heap/gc_allocation_budget.cpp
    heap/gc_remembered_set_manager.cpp
    heap/gc_heap_statistics.cpp
    heap/gc_finalization_executor.cpp
    heap/gc_heap_verifier.cpp

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
    builtins/reflect.cpp
    builtins/generator.cpp
    builtins/intl.cpp
    builtins/typed_array.cpp
    builtins/data_view.cpp
    builtins/temporal.cpp
    builtins/atomics.cpp

    # WebAssembly
    wasm/wasm.cpp
    wasm/WasmValidate.cpp
    wasm/WasmBaselineCompile.cpp
    wasm/WasmInterpreter.cpp
    wasm/WasmStackManager.cpp
    wasm/WasmSignalHandlers.cpp
    wasm/ZWasmFaultHandler.cpp

    # JIT
    jit/jit_profiler.cpp
    jit/type_profiler.cpp
    jit/baseline_jit.cpp
    jit/osr.cpp
    jit/macro_assembler.cpp
    jit/deoptimizer.cpp

    # Zepra Optimizer
    zopt/ZOptGraph.cpp
    zopt/ZOptBuilder.cpp
    zopt/ZOptLowering.cpp
    zopt/ZOptPipeline.cpp

    # Memory
    memory/page_allocator.cpp
    memory/arena_allocator.cpp
    memory/memory_pool.cpp

    # API
    api/isolate.cpp
    api/local_handle.cpp
    api/script.cpp
    api/value_api.cpp
    api/script_engine.cpp

    # Debugger (was debug/)
    debugger/debug_api.cpp
    debugger/breakpoint_manager.cpp
    debugger/call_stack_info.cpp
    debugger/execution_control.cpp
    debugger/debugger.cpp
    debugger/inspector.cpp
    debugger/profiler.cpp
    debugger/console.cpp

    # Profiler
    profiler/cpu_profiler.cpp
    profiler/heap_profiler.cpp
    profiler/sampling_profiler.cpp
    profiler/timeline.cpp

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
    browser/structured_clone.cpp
    browser/indexeddb.cpp
    browser/service_worker.cpp
    browser/websocket.cpp
    browser/performance.cpp
    browser/console_api.cpp
    browser/devtools.cpp

    # Regex Engine
    regex/regex_compiler.cpp
    regex/regex_engine.cpp

    # Threading
    threading/thread_pool.cpp

    # Optimization
    optimization/hidden_class.cpp
    optimization/inline_cache_opt.cpp
    optimization/property_table.cpp
    optimization/speculation.cpp

    # Workers
    workers/WorkerModuleLoader.cpp

    # Integration
    integration/VMModuleIntegration.cpp
)

# Create core library
add_library(zepra-core STATIC ${ZEPRA_CORE_SOURCES})

# Determine nxhttp path
set(ZEPRA_SOURCE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")
if(EXISTS "${CMAKE_SOURCE_DIR}/source/nxhttp/include")
    set(NXHTTP_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/source/nxhttp/include")
    set(NXHTTP_LIB "${CMAKE_SOURCE_DIR}/source/nxhttp/build/libnxhttp.a")
elseif(EXISTS "${ZEPRA_SOURCE_ROOT}/nxhttp/include")
    set(NXHTTP_INCLUDE_DIR "${ZEPRA_SOURCE_ROOT}/nxhttp/include")
    set(NXHTTP_LIB "${ZEPRA_SOURCE_ROOT}/nxhttp/build/libnxhttp.a")
else()
    message(WARNING "nxhttp not found - fetch.cpp will fail to build")
    set(NXHTTP_INCLUDE_DIR "")
    set(NXHTTP_LIB "")
endif()

# Include directories — the engine root itself is the include root
target_include_directories(zepra-core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${NXHTTP_INCLUDE_DIR}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link threading support
find_package(Threads REQUIRED)

target_link_libraries(zepra-core
    PUBLIC
        Threads::Threads
)

if(NXHTTP_LIB AND EXISTS "${NXHTTP_LIB}")
    target_link_libraries(zepra-core PUBLIC ${NXHTTP_LIB})
endif()

if(UNIX AND NOT APPLE)
    target_link_libraries(zepra-core PUBLIC dl)
endif()

target_compile_definitions(zepra-core
    PRIVATE
        ZEPRA_VERSION="${PROJECT_VERSION}"
        ZEPRA_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        ZEPRA_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        ZEPRA_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)
