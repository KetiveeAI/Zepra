

# ZebraScript Engine - Complete Architecture with Modular Inspector

## Core Principle: Your Own DevTools + Optional CDP Extension

```
┌─────────────────────────────────────────────────────────────┐
│              PRIMARY: Zepra DevTools (C++)                  │
│                                                             │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  Native UI: Qt/GTK/ImGui (Your Design, Your Brand)  │  │
│  │  • Console  • Debugger  • Sources  • Network        │  │
│  │  • Performance  • Memory  • Elements (DOM)          │  │
│  └──────────────────┬───────────────────────────────────┘  │
│                     │                                       │
│                     │ Direct Native API                     │
└─────────────────────┼───────────────────────────────────────┘
                      │
                      │
   ┌──────────────────▼──────────────────┐
   │   Zepra Debug Protocol (Native)     │ ← Your protocol
   │   - Direct C++ API calls            │
   │   - Zero overhead                   │
   │   - No JSON/WebSocket               │
   └──────────────────┬──────────────────┘
                      │
                      │
   ┌──────────────────▼──────────────────┐
   │    Core Debug API (Always present)  │
   │    - Breakpoints                    │
   │    - Call stack                     │
   │    - Variable inspection            │
   │    - Execution control              │
   └──────────────────┬──────────────────┘
                      │
                      │
   ┌──────────────────▼──────────────────┐
   │      ZepraScript Core Engine        │
   │   (VM, JIT, GC, Runtime, etc.)      │
   └─────────────────────────────────────┘


┌─────────────────────────────────────────────────────────────┐
│         OPTIONAL EXTENSION: CDP Compatibility Layer         │
│              (For 3rd party tools like Chrome)              │
│                                                             │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐     │
│  │ Chrome       │  │ VSCode       │  │ Any CDP      │     │
│  │ DevTools     │  │ Debugger     │  │ Client       │     │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘     │
│         │                  │                  │             │
│         └──────────────────┴──────────────────┘             │
│                            │                                │
│         ┌──────────────────▼──────────────────┐            │
│         │  libzepra-cdp-extension.so/.dll     │            │
│         │  (Optional, can be deleted)         │            │
│         │  - WebSocket server                 │            │
│         │  - CDP JSON protocol                │            │
│         │  - Translates CDP ↔ Native API      │            │
│         └──────────────────┬──────────────────┘            │
│                            │                                │
└────────────────────────────┼────────────────────────────────┘
                             │
                             │ Uses same Core Debug API
                             ↓
                    (connects to engine)
```

## Project Structure

```
zeprascript/
├── CMakeLists.txt
├── cmake/
│   ├── Config.cmake
│   ├── CompilerWarnings.cmake
│   ├── Sanitizers.cmake
│   ├── Dependencies.cmake
│   └── InspectorModule.cmake              # Optional module build
│
├── include/
│   └── zeprascript/
│       ├── config.hpp
│       ├── script_engine.hpp
│       ├── zepra_api.hpp                  # Main public API
│       │
│       ├── frontend/
│       │   ├── lexer.hpp
│       │   ├── token.hpp
│       │   ├── parser.hpp
│       │   ├── ast.hpp
│       │   ├── source_code.hpp
│       │   └── syntax_checker.hpp
│       │
│       ├── compiler/
│       │   ├── bytecode.hpp
│       │   ├── compiler.hpp
│       │   ├── optimizer.hpp
│       │   ├── constant_folder.hpp
│       │   ├── dead_code_eliminator.hpp
│       │   └── register_allocator.hpp
│       │
│       ├── runtime/
│       │   ├── value.hpp
│       │   ├── object.hpp
│       │   ├── function.hpp
│       │   ├── vm.hpp
│       │   ├── gc.hpp
│       │   ├── environment.hpp
│       │   ├── global_object.hpp
│       │   ├── prototype.hpp
│       │   ├── property_descriptor.hpp
│       │   ├── symbol.hpp
│       │   ├── iterator.hpp
│       │   ├── promise.hpp
│       │   ├── weak_map.hpp
│       │   ├── weak_set.hpp
│       │   ├── proxy.hpp
│       │   ├── reflect.hpp
│       │   └── module.hpp
│       │
│       ├── builtins/
│       │   ├── array.hpp
│       │   ├── string.hpp
│       │   ├── number.hpp
│       │   ├── boolean.hpp
│       │   ├── object_builtins.hpp
│       │   ├── function_builtins.hpp
│       │   ├── math.hpp
│       │   ├── date.hpp
│       │   ├── regexp.hpp
│       │   ├── json.hpp
│       │   ├── map.hpp
│       │   ├── set.hpp
│       │   ├── typed_array.hpp
│       │   ├── array_buffer.hpp
│       │   ├── data_view.hpp
│       │   └── console.hpp
│       │
│       ├── jit/
│       │   ├── jit_compiler.hpp
│       │   ├── baseline_jit.hpp
│       │   ├── dfg_jit.hpp
│       │   ├── ftl_jit.hpp
│       │   ├── assembler.hpp
│       │   ├── code_block.hpp
│       │   ├── call_frame.hpp
│       │   ├── register.hpp
│       │   ├── profiler.hpp
│       │   ├── type_profiler.hpp
│       │   ├── inline_cache.hpp
│       │   └── osr.hpp
│       │
│       ├── gc/
│       │   ├── heap.hpp
│       │   ├── allocator.hpp
│       │   ├── marking.hpp
│       │   ├── sweeping.hpp
│       │   ├── compacting.hpp
│       │   ├── incremental_gc.hpp
│       │   ├── concurrent_gc.hpp
│       │   ├── generational_gc.hpp
│       │   ├── write_barrier.hpp
│       │   ├── handle.hpp
│       │   ├── weak_ref.hpp
│       │   └── finalizer.hpp
│       │
│       ├── memory/
│       │   ├── memory_pool.hpp
│       │   ├── arena_allocator.hpp
│       │   ├── slab_allocator.hpp
│       │   ├── stack.hpp
│       │   └── page_allocator.hpp
│       │
│       ├── host/
│       │   ├── host_context.hpp
│       │   ├── native_function.hpp
│       │   ├── callback.hpp
│       │   ├── foreign_function_interface.hpp
│       │   ├── bindings_generator.hpp
│       │   └── type_traits.hpp
│       │
│       ├── browser/
│       │   ├── dom_bindings.hpp
│       │   ├── window_object.hpp
│       │   ├── document_object.hpp
│       │   ├── element_bindings.hpp
│       │   ├── event_system.hpp
│       │   ├── event_target.hpp
│       │   ├── event_listener.hpp
│       │   ├── xhr_bindings.hpp
│       │   ├── fetch_api.hpp
│       │   ├── websocket_bindings.hpp
│       │   ├── storage_api.hpp
│       │   ├── console_bindings.hpp
│       │   ├── timer_bindings.hpp
│       │   ├── url_api.hpp
│       │   ├── web_worker.hpp
│       │   └── service_worker.hpp
│       │
│       ├── debug/                             # ← Core debug hooks (minimal)
│       │   ├── debug_api.hpp                 # YOUR native debug protocol
│       │   ├── breakpoint_manager.hpp
│       │   ├── call_stack_info.hpp
│       │   ├── variable_inspector.hpp
│       │   ├── source_map.hpp
│       │   └── execution_control.hpp
│       │
│       ├── profiler/
│       │   ├── cpu_profiler.hpp
│       │   ├── heap_profiler.hpp
│       │   ├── sampling_profiler.hpp
│       │   └── timeline.hpp
│       │
│       ├── parser/
│       │   ├── parser_arena.hpp
│       │   ├── scope_analyzer.hpp
│       │   ├── variable_resolver.hpp
│       │   └── module_loader.hpp
│       │
│       ├── bytecode/
│       │   ├── bytecode_generator.hpp
│       │   ├── bytecode_instructions.hpp
│       │   ├── opcode.hpp
│       │   ├── jump_table.hpp
│       │   └── metadata.hpp
│       │
│       ├── interpreter/
│       │   ├── interpreter.hpp
│       │   ├── call_frame_manager.hpp
│       │   ├── stack_frame.hpp
│       │   └── exception_handler.hpp
│       │
│       ├── exception/
│       │   ├── exception.hpp
│       │   ├── error_object.hpp
│       │   ├── stack_trace.hpp
│       │   └── try_catch.hpp
│       │
│       ├── threading/
│       │   ├── thread_pool.hpp
│       │   ├── worker_thread.hpp
│       │   ├── lock.hpp
│       │   ├── atomic_ops.hpp
│       │   └── concurrent_queue.hpp
│       │
│       ├── async/
│       │   ├── event_loop.hpp
│       │   ├── microtask_queue.hpp
│       │   ├── task_queue.hpp
│       │   ├── promise_impl.hpp
│       │   └── async_context.hpp
│       │
│       ├── optimization/
│       │   ├── inline_cache.hpp
│       │   ├── polymorphic_cache.hpp
│       │   ├── hidden_class.hpp
│       │   ├── structure.hpp
│       │   ├── property_table.hpp
│       │   └── speculation.hpp
│       │
│       ├── api/
│       │   ├── context.hpp
│       │   ├── isolate.hpp
│       │   ├── persistent_handle.hpp
│       │   ├── local_handle.hpp
│       │   ├── template.hpp
│       │   ├── function_template.hpp
│       │   ├── object_template.hpp
│       │   └── signature.hpp
│       │
│       ├── modules/
│       │   ├── module_loader.hpp
│       │   ├── module_record.hpp
│       │   ├── import_resolver.hpp
│       │   ├── dynamic_import.hpp
│       │   └── module_namespace.hpp
│       │
│       ├── regex/
│       │   ├── regex_engine.hpp
│       │   ├── regex_compiler.hpp
│       │   ├── regex_bytecode.hpp
│       │   ├── regex_jit.hpp
│       │   └── unicode_support.hpp
│       │
│       └── utils/
│           ├── hash_table.hpp
│           ├── vector.hpp
│           ├── string_builder.hpp
│           ├── bit_vector.hpp
│           ├── assertions.hpp
│           ├── macros.hpp
│           ├── platform.hpp
│           └── unicode.hpp
│
├── cdp-extension/                           # ← OPTIONAL: CDP compatibility
│   ├── CMakeLists.txt                       # For 3rd party tools only
│   ├── README.md                            # "Optional CDP extension"
│   │
│   ├── include/
│   │   └── zeprascript/
│   │       └── cdp/
│   │           ├── cdp_server.hpp
│   │           ├── protocol_handler.hpp
│   │           ├── runtime_domain.hpp
│   │           ├── debugger_domain.hpp
│   │           ├── profiler_domain.hpp
│   │           └── cdp_translator.hpp       # Translates CDP ↔ Native API
│   │
│   └── src/
│       ├── cdp_server.cpp
│       ├── protocol_handler.cpp
│       ├── runtime_domain.cpp
│       ├── debugger_domain.cpp
│       ├── profiler_domain.cpp
│       └── cdp_translator.cpp
│
├── src/                                     # ← CORE ENGINE (required)
│   ├── main.cpp
│   ├── frontend/
│   │   ├── lexer.cpp
│   │   ├── token.cpp
│   │   ├── parser.cpp
│   │   ├── ast.cpp
│   │   ├── source_code.cpp
│   │   └── syntax_checker.cpp
│   │
│   ├── compiler/
│   │   ├── bytecode.cpp
│   │   ├── compiler.cpp
│   │   ├── optimizer.cpp
│   │   ├── constant_folder.cpp
│   │   ├── dead_code_eliminator.cpp
│   │   └── register_allocator.cpp
│   │
│   ├── runtime/
│   │   ├── value.cpp
│   │   ├── object.cpp
│   │   ├── function.cpp
│   │   ├── vm.cpp
│   │   ├── gc.cpp
│   │   ├── environment.cpp
│   │   ├── global_object.cpp
│   │   ├── prototype.cpp
│   │   ├── property_descriptor.cpp
│   │   ├── symbol.cpp
│   │   ├── iterator.cpp
│   │   ├── promise.cpp
│   │   ├── weak_map.cpp
│   │   ├── weak_set.cpp
│   │   ├── proxy.cpp
│   │   ├── reflect.cpp
│   │   └── module.cpp
│   │
│   ├── builtins/
│   │   ├── array.cpp
│   │   ├── string.cpp
│   │   ├── number.cpp
│   │   ├── boolean.cpp
│   │   ├── object_builtins.cpp
│   │   ├── function_builtins.cpp
│   │   ├── math.cpp
│   │   ├── date.cpp
│   │   ├── regexp.cpp
│   │   ├── json.cpp
│   │   ├── map.cpp
│   │   ├── set.cpp
│   │   ├── typed_array.cpp
│   │   ├── array_buffer.cpp
│   │   ├── data_view.cpp
│   │   └── console.cpp
│   │
│   ├── jit/
│   │   ├── jit_compiler.cpp
│   │   ├── baseline_jit.cpp
│   │   ├── dfg_jit.cpp
│   │   ├── ftl_jit.cpp
│   │   ├── assembler_x86.cpp
│   │   ├── assembler_x64.cpp
│   │   ├── assembler_arm.cpp
│   │   ├── assembler_arm64.cpp
│   │   ├── code_block.cpp
│   │   ├── call_frame.cpp
│   │   ├── register.cpp
│   │   ├── profiler.cpp
│   │   ├── type_profiler.cpp
│   │   ├── inline_cache.cpp
│   │   └── osr.cpp
│   │
│   ├── gc/
│   │   ├── heap.cpp
│   │   ├── allocator.cpp
│   │   ├── marking.cpp
│   │   ├── sweeping.cpp
│   │   ├── compacting.cpp
│   │   ├── incremental_gc.cpp
│   │   ├── concurrent_gc.cpp
│   │   ├── generational_gc.cpp
│   │   ├── write_barrier.cpp
│   │   ├── handle.cpp
│   │   ├── weak_ref.cpp
│   │   └── finalizer.cpp
│   │
│   ├── memory/
│   │   ├── memory_pool.cpp
│   │   ├── arena_allocator.cpp
│   │   ├── slab_allocator.cpp
│   │   ├── stack.cpp
│   │   └── page_allocator.cpp
│   │
│   ├── host/
│   │   ├── host_context.cpp
│   │   ├── native_function.cpp
│   │   ├── callback.cpp
│   │   ├── foreign_function_interface.cpp
│   │   ├── bindings_generator.cpp
│   │   ├── script_engine.cpp
│   │   └── type_traits.cpp
│   │
│   ├── browser/
│   │   ├── dom_bindings.cpp
│   │   ├── window_object.cpp
│   │   ├── document_object.cpp
│   │   ├── element_bindings.cpp
│   │   ├── event_system.cpp
│   │   ├── event_target.cpp
│   │   ├── event_listener.cpp
│   │   ├── xhr_bindings.cpp
│   │   ├── fetch_api.cpp
│   │   ├── websocket_bindings.cpp
│   │   ├── storage_api.cpp
│   │   ├── console_bindings.cpp
│   │   ├── timer_bindings.cpp
│   │   ├── url_api.cpp
│   │   ├── web_worker.cpp
│   │   └── service_worker.cpp
│   │
│   ├── debug/                               # ← Core debug API
│   │   ├── debug_api.cpp                   # YOUR native protocol
│   │   ├── breakpoint_manager.cpp
│   │   ├── call_stack_info.cpp
│   │   ├── variable_inspector.cpp
│   │   ├── source_map.cpp
│   │   └── execution_control.cpp
│   │
│   ├── profiler/
│   │   ├── cpu_profiler.cpp
│   │   ├── heap_profiler.cpp
│   │   ├── sampling_profiler.cpp
│   │   └── timeline.cpp
│   │
│   ├── parser/
│   │   ├── parser_arena.cpp
│   │   ├── scope_analyzer.cpp
│   │   ├── variable_resolver.cpp
│   │   └── module_loader.cpp
│   │
│   ├── bytecode/
│   │   ├── bytecode_generator.cpp
│   │   ├── bytecode_instructions.cpp
│   │   ├── opcode.cpp
│   │   ├── jump_table.cpp
│   │   └── metadata.cpp
│   │
│   ├── interpreter/
│   │   ├── interpreter.cpp
│   │   ├── call_frame_manager.cpp
│   │   ├── stack_frame.cpp
│   │   └── exception_handler.cpp
│   │
│   ├── exception/
│   │   ├── exception.cpp
│   │   ├── error_object.cpp
│   │   ├── stack_trace.cpp
│   │   └── try_catch.cpp
│   │
│   ├── threading/
│   │   ├── thread_pool.cpp
│   │   ├── worker_thread.cpp
│   │   ├── lock.cpp
│   │   ├── atomic_ops.cpp
│   │   └── concurrent_queue.cpp
│   │
│   ├── async/
│   │   ├── event_loop.cpp
│   │   ├── microtask_queue.cpp
│   │   ├── task_queue.cpp
│   │   ├── promise_impl.cpp
│   │   └── async_context.cpp
│   │
│   ├── optimization/
│   │   ├── inline_cache.cpp
│   │   ├── polymorphic_cache.cpp
│   │   ├── hidden_class.cpp
│   │   ├── structure.cpp
│   │   ├── property_table.cpp
│   │   └── speculation.cpp
│   │
│   ├── api/
│   │   ├── context.cpp
│   │   ├── isolate.cpp
│   │   ├── persistent_handle.cpp
│   │   ├── local_handle.cpp
│   │   ├── template.cpp
│   │   ├── function_template.cpp
│   │   ├── object_template.cpp
│   │   └── signature.cpp
│   │
│   ├── modules/
│   │   ├── module_loader.cpp
│   │   ├── module_record.cpp
│   │   ├── import_resolver.cpp
│   │   ├── dynamic_import.cpp
│   │   └── module_namespace.cpp
│   │
│   ├── regex/
│   │   ├── regex_engine.cpp
│   │   ├── regex_compiler.cpp
│   │   ├── regex_bytecode.cpp
│   │   ├── regex_jit.cpp
│   │   └── unicode_support.cpp
│   │
│   └── utils/
│       ├── hash_table.cpp
│       ├── vector.cpp
│       ├── string_builder.cpp
│       ├── bit_vector.cpp
│       ├── assertions.cpp
│       ├── platform.cpp
│       └── unicode.cpp
│
├── zepra-devtools/                          # ← YOUR OWN DevTools UI (primary)
│   ├── CMakeLists.txt                       # Native C++ UI with Qt/GTK
│   ├── include/
│   │   └── zepra_devtools/
│   │       ├── main_window.hpp
│   │       ├── console_panel.hpp
│   │       ├── debugger_panel.hpp
│   │       ├── sources_panel.hpp
│   │       ├── network_panel.hpp
│   │       ├── performance_panel.hpp
│   │       ├── memory_panel.hpp
│   │       ├── elements_panel.hpp          # DOM inspector
│   │       └── settings_panel.hpp
│   ├── src/
│   │   ├── main.cpp                        # Standalone DevTools app
│   │   ├── main_window.cpp
│   │   ├── console_panel.cpp
│   │   ├── debugger_panel.cpp
│   │   ├── sources_panel.cpp
│   │   ├── network_panel.cpp
│   │   ├── performance_panel.cpp
│   │   ├── memory_panel.cpp
│   │   ├── elements_panel.cpp
│   │   └── settings_panel.cpp
│   ├── ui/                                  # UI files
│   │   ├── main_window.ui
│   │   └── panels/*.ui
│   ├── resources/
│   │   ├── icons/
│   │   ├── themes/
│   │   └── styles/
│   └── README.md                            # "Zepra's Official DevTools"
│
├── tests/
│   ├── CMakeLists.txt
│   ├── unit/
│   │   ├── lexer_tests.cpp
│   │   ├── parser_tests.cpp
│   │   ├── vm_tests.cpp
│   │   ├── gc_tests.cpp
│   │   ├── jit_tests.cpp
│   │   ├── builtin_tests.cpp
│   │   ├── debug_api_tests.cpp             # Test YOUR native protocol
│   │   ├── devtools_integration_tests.cpp  # Test Zepra DevTools
│   │   └── api_tests.cpp
│   ├── integration/
│   │   ├── browser_integration_tests.cpp
│   │   ├── promise_tests.cpp
│   │   ├── module_tests.cpp
│   │   ├── worker_tests.cpp
│   │   └── inspector_tests.cpp            # Only if CDP extension enabled
│   └── test262/
│       └── runner.cpp
│
├── benchmarks/
│   ├── CMakeLists.txt
│   ├── bench_startup.cpp
│   ├── bench_execution.cpp
│   ├── bench_gc.cpp
│   ├── bench_jit.cpp
│   ├── sunspider/
│   ├── octane/
│   ├── jetstream/
│   └── speedometer/
│
├── tools/
│   ├── zepra-repl.cpp
│   ├── zepra-dump-bytecode.cpp
│   ├── zepra-jit-debug.cpp
│   ├── zepra-heap-snapshot.cpp
│   ├── zepra-devtools.cpp                  # Launch YOUR DevTools UI
│   ├── bindings-generator/
│   │   ├── generator.cpp
│   │   └── templates/
│   ├── devtools-launcher/                  # Launch Zepra DevTools
│   │   └── launcher.cpp
│   └── cdp-server/                         # Optional CDP server
│       └── server.cpp                      # Only if extension enabled
│
├── docs/
│   ├── architecture.md
│   ├── bytecode-spec.md
│   ├── jit-tiers.md
│   ├── gc-algorithm.md
│   ├── api-reference.md
│   ├── embedding-guide.md
│   ├── browser-integration.md
│   ├── performance-tuning.md
│   ├── native-debug-protocol.md            # YOUR debug protocol spec
│   ├── devtools-guide.md                   # Using Zepra DevTools
│   ├── cdp-extension.md                    # Optional CDP extension
│   └── contributing.md
│
├── third_party/
│   ├── double-conversion/
│   ├── icu/
│   ├── simdutf/
│   ├── qt/ or gtk/ or imgui/               # For Zepra DevTools UI
│   └── websocketpp/                        # Only for CDP extension
│
└── examples/
    ├── basic_embedding.cpp
    ├── dom_manipulation.cpp
    ├── custom_native_functions.cpp
    ├── worker_threads.cpp
    ├── browser_integration.cpp
    ├── using_zepra_devtools.cpp            # Using YOUR DevTools
    └── using_cdp_extension.cpp             # Optional CDP usage
```

## Minimal native scaffold to create now (files + dirs)

Create these first so the core JavaScript engine builds incrementally:

- `source/zepraScript/include/zeprascript/`
  - `config.hpp` – feature flags, platform toggles.
  - `script_engine.hpp` – public entry points to load/execute scripts.
  - `zepra_api.hpp` – embedding API surface exposed to host apps.
- `source/zepraScript/include/zeprascript/frontend/`
  - `lexer.hpp`, `token.hpp`, `parser.hpp`, `ast.hpp`, `source_code.hpp`, `syntax_checker.hpp`.
- `source/zepraScript/src/frontend/`
  - `lexer.cpp`, `parser.cpp`, `ast.cpp`, `source_code.cpp`, `syntax_checker.cpp`.
- `source/zepraScript/include/zeprascript/runtime/`
  - `vm.hpp`, `value.hpp`, `object.hpp`, `function.hpp`, `gc.hpp`, `environment.hpp`, `global_object.hpp`, `promise.hpp`, `module.hpp`.
- `source/zepraScript/src/runtime/`
  - Matching `.cpp` implementations for the headers above.
- `source/zepraScript/include/zeprascript/builtins/`
  - `array.hpp`, `string.hpp`, `number.hpp`, `object_builtins.hpp`, `function_builtins.hpp`, `math.hpp`, `date.hpp`, `console.hpp`.
- `source/zepraScript/src/builtins/`
  - Matching `.cpp` files implementing each builtin type.
- `source/zepraScript/include/zeprascript/gc/` and `src/gc/`
  - `heap.hpp/.cpp`, `allocator.hpp/.cpp`, `marking.hpp/.cpp`, `sweeping.hpp/.cpp`, `write_barrier.hpp/.cpp`.
- `source/zepraScript/include/zeprascript/api/` and `src/api/`
  - `context.hpp/.cpp`, `isolate.hpp/.cpp`, `persistent_handle.hpp/.cpp`, `local_handle.hpp/.cpp`, `function_template.hpp/.cpp`.
- `source/zepraScript/include/zeprascript/debug/` and `src/debug/`
  - `debug_api.hpp/.cpp`, `breakpoint_manager.hpp/.cpp`, `call_stack_info.hpp/.cpp`, `execution_control.hpp/.cpp`.
- `source/zepraScript/include/zeprascript/host/` and `src/host/`
  - `host_context.hpp/.cpp`, `native_function.hpp/.cpp`, `bindings_generator.hpp/.cpp`.
- `source/zepraScript/tests/`
  - `unit/` with `lexer_tests.cpp`, `parser_tests.cpp`, `vm_tests.cpp`, `gc_tests.cpp`, `builtin_tests.cpp`.
  - `integration/` with `module_tests.cpp`, `promise_tests.cpp`, `browser_integration_tests.cpp`.
- `source/zepraScript/tools/`
  - `zepra-repl.cpp` (CLI REPL), `zepra-dump-bytecode.cpp` (debug dump), `zepra-heap-snapshot.cpp` (GC inspection).
- `source/zepraScript/zepra-devtools/`
  - `include/` + `src/` stubs for `main_window`, panels, and Qt/GTK CMakeLists; links directly to `zepra-core`.
- `source/zepraScript/cdp-extension/` (optional)
  - `include/zeprascript/cdp/` + `src/` stubs for `cdp_server`, `protocol_handler`, `runtime_domain`, `debugger_domain`, `profiler_domain`, `cdp_translator`.

Quick start convention:
- Each header has `#pragma once` and declares the class/struct with TODOs.
- Each `.cpp` includes its matching header and provides minimal constructor/destructor definitions plus empty method bodies so the tree compiles.
- Add CMake targets as you add directories: core static library (`zepra-core`), optional `zepra-devtools`, optional `zepra-cdp-extension`, and unit test targets.

## Your Own Native Debug Protocol

### debug/debug_api.hpp
```cpp
namespace Zepra::Debug {

// YOUR native debug API - direct C++ calls, zero overhead
class DebugAPI {
public:
    // Breakpoint management
    struct Breakpoint {
        uint32_t id;
        std::string file;
        uint32_t line;
        std::string condition;  // Optional
        bool enabled;
    };
    
    uint32_t setBreakpoint(const std::string& file, uint32_t line,
                           const std::string& condition = "");
    void removeBreakpoint(uint32_t id);
    void toggleBreakpoint(uint32_t id);
    std::vector<Breakpoint> getAllBreakpoints() const;
    
    // Execution control
    void pause();
    void resume();
    void stepOver();
    void stepInto();
    void stepOut();
    
    // Call stack
    struct StackFrame {
        std::string function_name;
        std::string file;
        uint32_t line;
        uint32_t column;
        std::vector<Variable> locals;
        std::vector<Variable> closure_vars;
    };
    
    std::vector<StackFrame> getCallStack() const;
    
    // Variable inspection
    struct Variable {
        std::string name;
        std::string type;
        std::string value;
        bool expandable;
        std::vector<Variable> properties;  // If expandable
    };
    
    std::vector<Variable> getLocalVariables(uint32_t frame_index) const;
    std::vector<Variable> getGlobalVariables() const;
    Variable evaluateExpression(const std::string& expr, uint32_t frame_index);
    
    // Event callbacks (for your DevTools UI)
    using PausedCallback = std::function<void(const StackFrame& frame)>;
    using ConsoleCallback = std::function<void(const std::string& message, const std::string& level)>;
    using ExceptionCallback = std::function<void(const std::string& message, const StackFrame& frame)>;
    
    void onPaused(PausedCallback callback);
    void onConsoleMessage(ConsoleCallback callback);
    void onException(ExceptionCallback callback);
    
    // Performance profiling
    struct ProfileNode {
        std::string function;
        double self_time_ms;
        double total_time_ms;
        uint32_t call_count;
        std::vector<ProfileNode> children;
    };
    
    void startProfiling();
    ProfileNode stopProfiling();
    
    // Memory profiling
    struct HeapSnapshot {
        size_t total_size;
        size_t used_size;
        size_t object_count;
        std::vector<ObjectInfo> objects;
    };
    
    HeapSnapshot takeHeapSnapshot();
};

} // namespace Zepra::Debug
```

## Zepra DevTools UI (Your Primary Tool)

### zepra-devtools/include/zepra_devtools/main_window.hpp
```cpp
namespace Zepra::DevTools {

// YOUR DevTools - native C++ UI (Qt/GTK/ImGui)
class MainWindow : public QMainWindow {
    Q_OBJECT
    
public:
    explicit MainWindow(Zepra::Runtime::VM* vm);
    
private slots:
    void onPaused(const Zepra::Debug::StackFrame& frame);
    void onConsoleMessage(const QString& message, const QString& level);
    void onException(const QString& message);
    
private:
    // Direct connection to engine
    Zepra::Runtime::VM* vm_;
    Zepra::Debug::DebugAPI* debug_api_;
    
    // UI panels
    ConsolePanel* console_;
    DebuggerPanel* debugger_;
    SourcesPanel* sources_;
    NetworkPanel* network_;
    PerformancePanel* performance_;
    MemoryPanel* memory_;
    ElementsPanel* elements_;  // DOM inspector
    
    // No WebSocket, no JSON, just direct C++ calls!
};

} // namespace Zepra::DevTools
```

**Features of YOUR DevTools:**
- Native C++ performance (no web overhead)
- Direct API calls to engine (no protocol translation)
- Custom UI matching Zepra brand
- Integrated with ZepraBrowser
- No third-party dependencies
- Full control over features and design

## Optional CDP Extension (For 3rd Party Tools)

### cdp-extension/include/zeprascript/cdp/cdp_server.hpp
```cpp
namespace Zepra::CDP {

// OPTIONAL: Only for Chrome DevTools / VSCode compatibility
// Translates CDP (JSON/WebSocket) ↔ Native Debug API
class CDPServer {
public:
    explicit CDPServer(Zepra::Runtime::VM* vm, uint16_t port = 9222);
    
    void start();
    void stop();
    
    // Handles CDP JSON-RPC messages
    void handleCDPMessage(const std::string& method, const json& params);
    
private:
    Zepra::Runtime::VM* vm_;
    Zepra::Debug::DebugAPI* debug_api_;  // Uses YOUR native API
    WebSocketServer ws_server_;
    
    // CDP domain implementations
    RuntimeDomain runtime_;
    DebuggerDomain debugger_;
    ProfilerDomain profiler_;
    
    // Translates between CDP and your native protocol
    CDPTranslator translator_;
};

} // namespace Zepra::CDP
```

**Purpose of CDP Extension:**
- Allows developers to use Chrome DevTools (optional)
- Allows VSCode debugging (optional)
- Purely for compatibility
- **Can be completely deleted**
- Engine and Zepra DevTools work perfectly without it

## Build Configuration

### Root CMakeLists.txt
```cmake
cmake_minimum_required(VERSION 3.20)
project(ZepraScript VERSION 1.0.0 LANGUAGES CXX)

option(ZEPRA_BUILD_DEVTOOLS "Build Zepra DevTools UI" ON)
option(ZEPRA_BUILD_CDP_EXTENSION "Build CDP extension (for 3rd party tools)" OFF)

# Core engine (always built)
add_subdirectory(src)

# YOUR DevTools (primary debugging tool)
if(ZEPRA_BUILD_DEVTOOLS)
    find_package(Qt6 COMPONENTS Widgets REQUIRED)  # or GTK, or ImGui
    add_subdirectory(zepra-devtools)
endif()

# Optional CDP extension (for Chrome/VSCode compatibility)
if(ZEPRA_BUILD_CDP_EXTENSION)
    add_subdirectory(cdp-extension)
endif()

# Tests
add_subdirectory(tests)
```

### zepra-devtools/CMakeLists.txt
```cmake
# Your native DevTools application

find_package(Qt6 COMPONENTS Widgets REQUIRED)

add_executable(zepra-devtools
    src/main.cpp
    src/main_window.cpp
    src/console_panel.cpp
    src/debugger_panel.cpp
    src/sources_panel.cpp
    src/network_panel.cpp
    src/performance_panel.cpp
    src/memory_panel.cpp
    src/elements_panel.cpp
)

target_link_libraries(zepra-devtools
    PRIVATE
        zepra-core              # Direct link to engine
        Qt6::Widgets
)

# No WebSocket, no JSON parsing, just native C++ API calls!
```

### cdp-extension/CMakeLists.txt
```cmake
# Optional CDP compatibility layer - can be deleted

find_package(websocketpp REQUIRED)
find_package(nlohmann_json REQUIRED)

add_library(zepra-cdp-extension SHARED
    src/cdp_server.cpp
    src/protocol_handler.cpp
    src/runtime_domain.cpp
    src/debugger_domain.cpp
    src/profiler_domain.cpp
    src/cdp_translator.cpp
)

target_link_libraries(zepra-cdp-extension
    PRIVATE
        zepra-core              # Uses native debug API
        websocketpp::websocketpp
        nlohmann_json::nlohmann_json
)

# This module translates CDP ↔ Your Native API
# If deleted, engine and Zepra DevTools continue working perfectly