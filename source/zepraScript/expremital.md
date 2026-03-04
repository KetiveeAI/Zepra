# ZepraScript Engine — Architecture

> A high-performance JavaScript engine designed to power the Zepra browser.  
> Target scale: 2M+ lines of C++. Built for correctness, speed, and embedder flexibility.

---

## Table of Contents

1. [High-Level Overview](#1-high-level-overview)
2. [Repository Layout](#2-repository-layout)
3. [Subsystem Deep Dives](#3-subsystem-deep-dives)
   - [Frontend](#31-frontend)
   - [Compiler](#32-compiler)
   - [Bytecode](#33-bytecode)
   - [Interpreter](#34-interpreter)
   - [JIT Tiers](#35-jit-tiers)
   - [Runtime](#36-runtime)
   - [Builtins](#37-builtins)
   - [GC](#38-gc)
   - [Memory](#39-memory)
   - [Async & Event Loop](#310-async--event-loop)
   - [Optimization](#311-optimization)
   - [Modules](#312-modules)
   - [Regex](#313-regex)
   - [Threading](#314-threading)
   - [Browser Bindings](#315-browser-bindings)
   - [Host / Embedder API](#316-host--embedder-api)
   - [Debug](#317-debug)
   - [Profiler](#318-profiler)
   - [Exception Handling](#319-exception-handling)
   - [Utils](#320-utils)
4. [Public API Surface](#4-public-api-surface)
5. [Optional Extensions](#5-optional-extensions)
   - [CDP Extension](#51-cdp-extension)
6. [Zepra DevTools](#6-zepra-devtools)
7. [Execution Pipeline](#7-execution-pipeline)
8. [JIT Tier Escalation](#8-jit-tier-escalation)
9. [GC Strategy](#9-gc-strategy)
10. [Build System](#10-build-system)
11. [Testing Strategy](#11-testing-strategy)
12. [Benchmarking](#12-benchmarking)
13. [Design Principles](#13-design-principles)
14. [Dependency Map](#14-dependency-map)

---

## 1. High-Level Overview

ZepraScript is a multi-tier JavaScript engine with the following execution pipeline:

```
Source Code
    │
    ▼
[ Frontend ]  ──  Lexer → Parser → AST → Syntax Checker
    │
    ▼
[ Compiler ]  ──  Scope Analysis → Bytecode Generation → Peephole Optimizer
    │
    ▼
[ Interpreter ]  ──  Bytecode dispatch loop, baseline profiling
    │
    ▼  (hot path detected)
[ Baseline JIT ]  ──  Fast unoptimized native code, inline cache stubs
    │
    ▼  (type feedback collected)
[ DFG JIT ]  ──  Speculative optimization, type-specialized IR
    │
    ▼  (critical hot loops)
[ FTL / B3 JIT ]  ──  LLVM-style backend, aggressive inlining & scheduling
    │
    ▼
[ Runtime ]  ──  Object model, GC, async, modules, builtins
```

The engine is designed to be embedded inside the Zepra browser and also exposed as a standalone library for third-party embedders via a V8-compatible handle/isolate API.

---

## 2. Repository Layout

```
zeprascript/
├── include/zeprascript/       # Public headers — all embedder-facing API
├── src/                       # Core engine implementation
├── cdp-extension/             # Optional: Chrome DevTools Protocol adapter
├── zepra-devtools/            # Zepra's native DevTools UI (Qt/GTK)
├── tests/                     # Unit, integration, Test262
├── benchmarks/                # JetStream, Speedometer, Octane, custom
├── tools/                     # REPL, bytecode dumper, heap snapshot, etc.
├── docs/                      # Specs, guides, protocol definitions
├── third_party/               # ICU, double-conversion, simdutf, UI framework
├── examples/                  # Embedding examples
└── cmake/                     # Build configuration modules
```

The `src/` and `include/zeprascript/` directories mirror each other subsystem-for-subsystem. Every internal `.cpp` has a corresponding public `.hpp` only if that interface is part of the embedder surface — internal-only types stay header-private.

---

## 3. Subsystem Deep Dives

### 3.1 Frontend

**Location:** `src/frontend/`, `include/zeprascript/frontend/`

Responsible for transforming raw source text into a typed AST.

| File | Role |
|---|---|
| `lexer` | Tokenizes UTF-8 source, handles Unicode identifiers, template literals, regex literals |
| `token` | Token type definitions and metadata |
| `parser` | Recursive descent parser, produces full ES2025 AST |
| `ast` | AST node types, visitor interface |
| `source_code` | Source buffer management, line/col mapping |
| `syntax_checker` | Early error detection per spec (strict mode, duplicate params, etc.) |

The parser performs no semantic analysis — scope resolution is a separate pass in `src/parser/` (scope analyzer, variable resolver). This keeps the grammar-level parse fast and stateless.

### 3.2 Compiler

**Location:** `src/compiler/`, `include/zeprascript/compiler/`

Transforms the AST into bytecode via a single-pass compiler with a post-pass optimizer.

| File | Role |
|---|---|
| `compiler` | Main AST-to-bytecode lowering pass |
| `bytecode` | Bytecode instruction set definitions |
| `optimizer` | Driver for all optimization passes |
| `constant_folder` | Folds compile-time constant expressions |
| `dead_code_eliminator` | Removes provably unreachable code |
| `register_allocator` | Virtual register allocation for the bytecode VM |

The compiler targets a register-based bytecode VM (not stack-based), which reduces dispatch overhead and simplifies JIT lifting.

### 3.3 Bytecode

**Location:** `src/bytecode/`, `include/zeprascript/bytecode/`

Defines and manages the engine's instruction set.

| File | Role |
|---|---|
| `opcode` | Opcode enum, instruction widths, operand formats |
| `bytecode_generator` | Low-level instruction emitter used by the compiler |
| `bytecode_instructions` | Per-instruction semantics documentation and dispatch tables |
| `jump_table` | Computed goto table for threaded dispatch |
| `metadata` | Per-instruction profiling metadata (type feedback slots) |

### 3.4 Interpreter

**Location:** `src/interpreter/`, `include/zeprascript/interpreter/`

The baseline execution tier. All code starts here.

- Threaded dispatch loop using computed gotos (where supported)
- Maintains type feedback metadata per call site for JIT profiling
- `call_frame_manager` handles stack frame allocation and unwinding
- `exception_handler` implements `try/catch/finally` and exception propagation

### 3.5 JIT Tiers

**Location:** `src/jit/`, `include/zeprascript/jit/`

Three compilation tiers of increasing optimization depth:

**Baseline JIT**
- Triggered after a function exceeds a low invocation threshold
- Emits unoptimized native code with inline cache stubs
- Inline caches cover property access, method calls, arithmetic operators
- Fast to compile — latency is the priority, not throughput

**DFG JIT (Data Flow Graph)**
- Triggered by type feedback from baseline
- Builds a sea-of-nodes IR from bytecode
- Performs type specialization, inlining, escape analysis
- Includes OSR (On-Stack Replacement) for loop-heavy code

**FTL JIT (Faster Than Light) / B3 Backend**
- Triggered for the hottest functions under sustained load
- Lowers DFG IR to B3 (a low-level IR similar to LLVM IR)
- Full register allocation, instruction scheduling, SIMD where applicable
- Deoptimization support: bails back to interpreter on speculation failure

**Platform Assemblers:** `assembler_x64`, `assembler_arm64`, `assembler_x86`, `assembler_arm` — one per target arch.

### 3.6 Runtime

**Location:** `src/runtime/`, `include/zeprascript/runtime/`

The core JS object model and VM runtime.

| File | Role |
|---|---|
| `value` | Tagged value representation (NaN-boxing or pointer tagging) |
| `object` | Base JS object, property storage, prototype chain |
| `function` | JS function objects, closures, bound functions |
| `vm` | VM state, execution context, call dispatch |
| `environment` | Lexical scope records, variable binding |
| `global_object` | The global object and its initial property population |
| `prototype` | Prototype chain traversal and manipulation |
| `symbol` | Symbol registry and well-known symbols |
| `iterator` | Iterator protocol implementation |
| `promise` | Promise state machine |
| `proxy` | Proxy/Reflect trap dispatch |
| `module` | Module record, namespace objects |
| `weak_map / weak_set` | Ephemeron-based weak collections |

### 3.7 Builtins

**Location:** `src/builtins/`, `include/zeprascript/builtins/`

Native implementations of all ECMAScript built-in objects and methods. Each file corresponds to one built-in namespace:

`Array`, `String`, `Number`, `Boolean`, `Object`, `Function`, `Math`, `Date`, `RegExp`, `JSON`, `Map`, `Set`, `TypedArray`, `ArrayBuffer`, `DataView`, `Console`

Builtins are implemented as native C++ functions registered into the global object at engine startup. Performance-critical paths (e.g. `Array.prototype.map`, `String.prototype.indexOf`) have JIT-optimized intrinsic variants.

### 3.8 GC

**Location:** `src/gc/`, `include/zeprascript/gc/`

A generational, incremental, concurrent garbage collector.

| Component | Description |
|---|---|
| **Nursery** | Young generation — bump-pointer allocation, frequent minor GC |
| **Old Generation** | Tenured objects — mark-sweep-compact |
| **Incremental GC** | Slices major GC work across multiple JS frames to avoid long pauses |
| **Concurrent GC** | Background marking thread runs concurrently with JS execution |
| **Write Barrier** | Dijkstra-style write barrier to maintain heap invariants during concurrent marking |
| **Compaction** | Optional compacting phase to reduce heap fragmentation |
| **Handles** | Rooted handle system for GC-safe object references from C++ |
| **Finalizers** | Post-collection callbacks for objects with native resources |
| **Weak Refs** | GC-aware weak references per the WeakRef/FinalizationRegistry spec |

### 3.9 Memory

**Location:** `src/memory/`, `include/zeprascript/memory/`

Low-level allocators below the GC layer.

| Allocator | Use Case |
|---|---|
| `page_allocator` | OS-level memory mapping (mmap/VirtualAlloc) |
| `arena_allocator` | Region-based allocation for AST nodes and compiler temporaries |
| `slab_allocator` | Fixed-size object allocation for common heap types |
| `memory_pool` | General-purpose pool with size classes |

### 3.10 Async & Event Loop

**Location:** `src/async/`, `include/zeprascript/async/`

| File | Role |
|---|---|
| `event_loop` | Main event loop — integrates with browser's native event system |
| `microtask_queue` | Promise resolution queue, runs to completion after each task |
| `task_queue` | Macrotask queue (setTimeout, I/O callbacks) |
| `promise_impl` | Internal Promise state machine implementation |
| `async_context` | AsyncLocalStorage-style context propagation |

The event loop is designed to be driven externally by the browser host — ZepraScript exposes `pump()` and `drain_microtasks()` hooks rather than owning the loop itself.

### 3.11 Optimization

**Location:** `src/optimization/`, `include/zeprascript/optimization/`

Runtime optimization infrastructure shared across JIT tiers.

| File | Role |
|---|---|
| `hidden_class` | Shape/hidden class transitions for fast property access |
| `structure` | Structural sharing between object shapes |
| `property_table` | Hash table for object property storage |
| `inline_cache` | Monomorphic and polymorphic inline caches |
| `polymorphic_cache` | PIC with up to N cached shapes before megamorphic fallback |
| `speculation` | Type speculation guards and deopt triggers |

### 3.12 Modules

**Location:** `src/modules/`, `include/zeprascript/modules/`

Full ES module system with dynamic import support.

- `module_loader` — resolves specifiers to source, handles caching
- `module_record` — per-module parse/evaluation state
- `import_resolver` — pluggable resolver (browser URL semantics vs Node path semantics)
- `dynamic_import` — `import()` expression support with promise integration
- `module_namespace` — live binding namespace objects

### 3.13 Regex

**Location:** `src/regex/`, `include/zeprascript/regex/`

Self-contained regex engine with JIT compilation.

- `regex_compiler` — compiles regex syntax to regex bytecode
- `regex_engine` — NFA/DFA hybrid interpreter for regex bytecode  
- `regex_bytecode` — regex-specific instruction set
- `regex_jit` — compiles hot regex patterns to native code
- `unicode_support` — Unicode property escapes, case folding, normalization

### 3.14 Threading

**Location:** `src/threading/`, `include/zeprascript/threading/`

Threading primitives used by the GC, JIT compiler, and Web Workers.

- `thread_pool` — fixed-size worker thread pool for background JIT compilation and GC
- `worker_thread` — individual worker lifecycle management
- `concurrent_queue` — lock-free MPSC queue for cross-thread work items
- `atomic_ops` — platform-portable atomic primitives
- `lock` — mutex/spinlock abstractions

JS execution itself is single-threaded per isolate. Threading here is for background engine work only. Web Worker isolation is managed separately in `browser/web_worker`.

### 3.15 Browser Bindings

**Location:** `src/browser/`, `include/zeprascript/browser/`

Web platform API implementations that bind to the JS runtime.

Organized by web platform surface:

- **DOM:** `dom_bindings`, `document_object`, `element_bindings`, `window_object`
- **Events:** `event_system`, `event_target`, `event_listener`
- **Networking:** `fetch_api`, `xhr_bindings`, `websocket_bindings`
- **Storage:** `storage_api`
- **Workers:** `web_worker`, `service_worker`
- **Timers:** `timer_bindings`
- **Other:** `url_api`, `console_bindings`

> **Note:** At 2M+ LOC scale, `browser/` should be split into `web/dom/`, `web/events/`, `web/networking/` subdirectories.

### 3.16 Host / Embedder API

**Location:** `src/host/`, `include/zeprascript/host/`

The layer between ZepraScript and the Zepra browser (or any third-party embedder).

- `host_context` — host-provided execution context and callbacks
- `native_function` — wrapping C++ functions as callable JS values
- `callback` — JS-to-C++ and C++-to-JS callback management
- `foreign_function_interface` — structured FFI for complex type marshaling
- `bindings_generator` — code-gen tool for automatic binding generation
- `type_traits` — compile-time type mapping between C++ and JS types

### 3.17 Debug

**Location:** `src/debug/`, `include/zeprascript/debug/`

ZepraScript's **native** debug protocol — independent of CDP.

| File | Role |
|---|---|
| `debug_api` | Primary debug interface: attach, detach, set hooks |
| `breakpoint_manager` | Source-level breakpoint registration and hit detection |
| `call_stack_info` | Stack frame inspection during a pause |
| `variable_inspector` | Scoped variable enumeration and value retrieval |
| `source_map` | Source map parsing and position translation |
| `execution_control` | Step-in, step-over, step-out, continue, pause |

This is your protocol. Tools that want CDP compatibility go through the optional `cdp-extension/` adapter, not through this layer.

### 3.18 Profiler

**Location:** `src/profiler/`, `include/zeprascript/profiler/`

| File | Role |
|---|---|
| `cpu_profiler` | Sampled CPU profiling with call tree construction |
| `heap_profiler` | Heap snapshot generation, allocation tracking |
| `sampling_profiler` | Statistical sampling at configurable intervals |
| `timeline` | Event timeline for DevTools performance panel |

### 3.19 Exception Handling

**Location:** `src/exception/`, `include/zeprascript/exception/`

- `exception` — base exception type, JS throw/catch integration
- `error_object` — `Error`, `TypeError`, `RangeError`, etc.
- `stack_trace` — V8-compatible stack trace capture
- `try_catch` — C++ TryCatch scope for embedder exception handling

### 3.20 Utils

**Location:** `src/utils/`, `include/zeprascript/utils/`

Foundational types used engine-wide.

`hash_table`, `vector`, `string_builder`, `bit_vector`, `assertions`, `macros`, `platform`, `unicode`

These have no dependencies on any engine subsystem and can be used freely across all layers.

---

## 4. Public API Surface

**Location:** `include/zeprascript/api/`, `include/zeprascript/zepra_api.hpp`

The embedder-facing API, modeled after V8's handle-based design:

| Concept | Description |
|---|---|
| `Isolate` | An isolated JS heap and engine instance |
| `Context` | A JS execution context (global scope) within an isolate |
| `LocalHandle<T>` | Stack-scoped GC-safe reference to a JS value |
| `PersistentHandle<T>` | Heap-allocated long-lived GC-safe reference |
| `FunctionTemplate` | Blueprint for creating native-backed JS functions |
| `ObjectTemplate` | Blueprint for creating native-backed JS objects |
| `Signature` | Call signature for function template validation |

Embedders `#include <zeprascript/zepra_api.hpp>` and nothing else. All internal headers are non-public.

---

## 5. Optional Extensions

### 5.1 CDP Extension

**Location:** `cdp-extension/`

An **optional**, separately compiled module that wraps the native debug API with a Chrome DevTools Protocol (CDP) server. Intended for compatibility with third-party tooling (VS Code, Puppeteer, etc.) — not for Zepra's own DevTools.

```
cdp-extension/
├── include/zeprascript/cdp/
│   ├── cdp_server.hpp
│   ├── protocol_handler.hpp
│   ├── cdp_translator.hpp        # Translates CDP ↔ Native Debug API
│   ├── runtime_domain.hpp
│   ├── debugger_domain.hpp
│   └── profiler_domain.hpp
└── src/
    └── (implementations)
```

Enable with: `cmake -DZEPRA_CDP_EXTENSION=ON ..`

The CDP layer sits **entirely above** the native debug API. Changes to the debug protocol do not require touching CDP code.

---

## 6. Zepra DevTools

**Location:** `zepra-devtools/`

Zepra's first-class native DevTools application. Built with Qt or GTK, communicates directly with the native debug API — no CDP round-trip.

```
zepra-devtools/
├── src/
│   ├── main_window.cpp
│   ├── console_panel.cpp        # JS console with REPL
│   ├── debugger_panel.cpp       # Breakpoints, step, call stack, variables
│   ├── sources_panel.cpp        # Source viewer with source map support
│   ├── network_panel.cpp        # Fetch/XHR/WebSocket inspector
│   ├── performance_panel.cpp    # CPU timeline, flame charts
│   ├── memory_panel.cpp         # Heap snapshots, allocation tracker
│   └── elements_panel.cpp       # DOM inspector
├── ui/                          # .ui layout files
└── resources/                   # Icons, themes, stylesheets
```

---

## 7. Execution Pipeline

```
                     ┌──────────────┐
   Source text ───▶  │   Frontend   │  Lex → Parse → AST → Syntax check
                     └──────┬───────┘
                            │ AST
                     ┌──────▼───────┐
                     │   Compiler   │  Scope resolution → Bytecode emit
                     └──────┬───────┘
                            │ Bytecode
                     ┌──────▼───────┐
                     │ Interpreter  │  Execute + collect type feedback
                     └──────┬───────┘
                   hot?     │
              ┌─────────────▼─────────────┐
              │       Baseline JIT        │  Unoptimized native + IC stubs
              └─────────────┬─────────────┘
                   hot?     │ type feedback
              ┌─────────────▼─────────────┐
              │          DFG JIT          │  Speculative + type-specialized
              └─────────────┬─────────────┘
                   hot?     │
              ┌─────────────▼─────────────┐
              │        FTL / B3           │  Maximum optimization
              └───────────────────────────┘
                            │  deopt?
                            └──────────▶  Interpreter (bail-out)
```

---

## 8. JIT Tier Escalation

| Tier | Trigger | Compile Time | Peak Throughput |
|---|---|---|---|
| Interpreter | Always | None | Baseline |
| Baseline JIT | ~100 calls or loop iterations | ~1ms | 2–5× interpreter |
| DFG JIT | ~1000 calls + type stability | ~10ms | 10–30× interpreter |
| FTL / B3 | ~10000 calls + DFG hotness | ~50ms | 50–100× interpreter |

Thresholds are tunable at runtime via engine configuration.

**Deoptimization** occurs when a speculation assumption is invalidated (e.g. a property type changes). The engine bails back to interpreter with reconstructed stack state and resets the profiling counter for that function.

---

## 9. GC Strategy

```
Heap Layout:

  ┌──────────────────────────────────────────────────────┐
  │                     Nursery (Young Gen)              │
  │    bump-pointer allocation │ semi-space copy GC      │
  └──────────────────────────────────────────────────────┘
  ┌──────────────────────────────────────────────────────┐
  │                  Old Generation                      │
  │    mark-sweep-compact │ incremental │ concurrent     │
  └──────────────────────────────────────────────────────┘
  ┌──────────────────────────────────────────────────────┐
  │               Large Object Space                     │
  │    directly mapped pages │ no compaction             │
  └──────────────────────────────────────────────────────┘
```

- **Minor GC:** Nursery collection, runs frequently (~every few MB allocated), pauses < 1ms
- **Major GC:** Old gen collection, incremental slices keep individual pauses < 5ms
- **Concurrent marking:** Background thread marks reachable objects while JS runs
- **Write barrier:** Every pointer store through a write barrier to track cross-generation references
- **Compaction:** Runs on demand to reduce fragmentation, moves objects to contiguous ranges

---

## 10. Build System

**CMake 3.20+** with Ninja as the recommended generator.

```bash
# Configure
cmake -B build_ninja -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DZEPRA_CDP_EXTENSION=OFF \
  -DZEPRA_DEVTOOLS=ON

# Build
cmake --build build_ninja

# Test
ctest --test-dir build_ninja
```

Key CMake modules in `cmake/`:

| Module | Purpose |
|---|---|
| `CompilerWarnings.cmake` | Warning flags per compiler |
| `Sanitizers.cmake` | ASan/TSan/UBSan build targets |
| `Dependencies.cmake` | Third-party dependency resolution |
| `InspectorModule.cmake` | Optional CDP extension build |

---

## 11. Testing Strategy

```
tests/
├── unit/           # Per-subsystem C++ unit tests (Catch2 / GoogleTest)
├── integration/    # Cross-subsystem tests (browser, promises, modules, workers)
└── test262/        # ECMAScript conformance suite runner
```

| Suite | What It Covers |
|---|---|
| Unit tests | Lexer, parser, VM, GC, JIT, builtins, debug API, API surface |
| Integration tests | Browser bindings, module loading, promise chains, Web Workers |
| Test262 | Full ECMAScript spec conformance — target 99%+ pass rate |
| Benchmarks | JetStream2, Speedometer, Octane, custom micro-benchmarks |

CI enforces: no regression in Test262 pass rate, no performance regression > 2% on JetStream.

---

## 12. Benchmarking

```
benchmarks/
├── bench_startup.cpp      # Cold start time
├── bench_execution.cpp    # General execution throughput
├── bench_gc.cpp           # GC pause times and throughput
├── bench_jit.cpp          # JIT compilation latency and peak perf
├── sunspider/
├── octane/
├── jetstream/
└── speedometer/
```

Run with: `./build_ninja/bin/zepra-bench --suite jetstream`

---

## 13. Design Principles

**1. Correctness first, speed second.**  
The interpreter is the source of truth for semantics. JIT tiers must produce identical observable behavior.

**2. The engine does not own the event loop.**  
ZepraScript exposes hooks. The browser drives execution.

**3. The native debug protocol is the primary protocol.**  
CDP is a compatibility adapter. Internal tooling uses the native protocol directly.

**4. No subsystem depends on `browser/`.**  
Browser bindings are a leaf layer. Core engine (runtime, GC, JIT) has no knowledge of DOM or web APIs.

**5. Public API is stable; internals are not.**  
Everything under `include/zeprascript/api/` and `zepra_api.hpp` follows semver. Internal headers can change freely.

**6. All allocations go through the memory layer.**  
No raw `new`/`malloc` in engine code. Everything routes through arena, slab, or pool allocators, or the GC heap.

---

## 14. Dependency Map

```
utils         ←  no engine deps (foundational)
memory        ←  utils
gc            ←  memory, utils
runtime       ←  gc, memory, utils
builtins      ←  runtime
interpreter   ←  bytecode, runtime
compiler      ←  frontend, bytecode, runtime
jit           ←  interpreter, optimization, runtime, memory
optimization  ←  runtime, gc
async         ←  runtime (promise), threading
modules       ←  compiler, runtime
regex         ←  utils, memory
exception     ←  runtime
browser       ←  runtime, builtins, async, modules        ← LEAF
host          ←  runtime, gc                              ← LEAF
debug         ←  runtime, interpreter                     ← LEAF
profiler      ←  runtime, jit, gc                        ← LEAF
cdp-extension ←  debug                                   ← OPTIONAL LEAF
zepra-devtools←  debug, profiler                         ← OPTIONAL LEAF
```

Circular dependencies between core subsystems are forbidden. Any proposal that would introduce a cycle must be resolved by extracting a shared interface layer first.

---

*This document should be updated alongside any major subsystem addition or API change.*  
*For the bytecode instruction reference see `docs/bytecode-spec.md`. For JIT internals see `docs/jit-tiers.md`.*