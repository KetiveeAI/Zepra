# ZepraScript Development Status

> **Last Updated:** 2026-03-01
> **Version:** 1.0.1
> **Build:** GCC 13.3.0, C++20
> **Tests:** 266/267 passing

---

## Completed Phases

### Phase 1–35: Core Foundation ✅

- Value system (NaN-boxing), GC, Lexer, Parser, Bytecode, VM
- ES2024 API headers (Array, Object, Promise, Temporal, Intl, etc.)
- WASM 2.0+ headers (SIMD, GC, Components, Exception Handling)
- Baseline JIT, Inline Cache, Type Specialization headers
- Browser Integration headers (DOM, Fetch, Workers, DevTools)

### Phase 36–45: Infrastructure & 1.0 Release ✅

- Spec test harness, performance benchmarks
- GC enhancement headers (WriteBarrier, Compaction, Nursery, OldGeneration, GCController)
- JIT low-level headers (MacroAssembler, CodePatching, Deoptimization, RegAlloc)
- Bytecode expansion headers (OpcodeReference, BytecodeOptimizer, BytecodeSerialization)
- VM infrastructure headers (StackFrame, Interpreter, ExecutionContext, VMInternals)
- Debugger/Profiler headers (BreakpointManager, SourceMapper, StepDebugger, HotSpotTracker)
- Runtime headers (Shape, NativeBinding, PropertyStorage, ObjectLayout)

### Phase 46: Core Stabilization ✅ (Implemented)

- [x] `utils/unicode.cpp` — UTF-8/UTF-16 codec, codepoint operations, Unicode categories
- [x] `utils/string_builder.cpp` — chunked rope-style string builder
- [x] `utils/platform.cpp` — high-res timer, platform detection, page size, hw concurrency
- [x] `frontend/syntax_checker.cpp` — const-init, break/continue/return, duplicate let/const
- [x] GC safe-points wired into VM `run()` loop
- [x] Write barriers in `OP_SET_PROPERTY` and `OP_SET_ELEMENT`
- [x] `GCHeap*` member added directly to VM class

### Phase 47: WASM Stabilization ✅ (Implemented)

- [x] `WasmInstance::buildExports()` — exports functions, memories, tables, globals as JS values
- [x] Init expression evaluation — proper LEB128 signed decoder
- [x] `WasmInstance::executeStartFunction()` — invokes WasmInterpreter
- [x] Data segment memory copy bounds check
- [x] `I32_CONST` / `I64_CONST` instruction dispatch — proper signed LEB128

### Phase 48: Stub Cleanup ✅ (Implemented)

- [x] `interpreter/interpreter.cpp` — debug stepping (StepInto/StepOver/StepOut)
- [x] `interpreter/stack_frame.cpp` — call stack inspector, Error.stack formatting
- [x] `memory/memory_pool.cpp` — fixed-size slab allocator with O(1) alloc/dealloc
- [x] `host/native_function.cpp` — C++↔JS function binding, registerGlobalFunction/registerMethod
- [x] `host/script_engine.cpp` — full embedding pipeline (lex→parse→check→compile→execute)

### Phase 49: GC Hardening ✅ (Implemented)

- [x] Incremental mark dedup — O(1) hash set instead of O(n) linear scan
- [x] Incremental sweep — delegates actual deletion to heap
- [x] Mark clearing on survivors for next-cycle correctness

### Phase 50: BigInt & Value Integration ✅ (Implemented)

- [x] `bigint_object.hpp` — heap-allocated BigInt wrapper with ObjectType::BigInt
- [x] `value.hpp` / `value.cpp` — `isBigInt()`, `asBigInt()`, `bigint()` factory
- [x] `BigIntAPI.h` — fixed `operator~()` (use negate() instead of missing unary -)
- [x] `BigInt` added to `ObjectType` enum and `ValueType` enum

---

## Current Position

**Phase 50 COMPLETE** — Core VM stabilized with real implementations

### Remaining Small Stubs

| File                        | Lines | Status                              |
| --------------------------- | ----- | ----------------------------------- |
| `runtime/async.cpp`         | 8     | Stub — needs async/await VM support |
| `frontend/ast.cpp`          | 9     | Stub — AST utilities                |
| `wasm/WasmStackManager.cpp` | 12    | Stub — WASM stack management        |

### 88 Header-Only APIs

Declarations exist for Temporal, Decorators, Pipeline, WeakRef, FinalizationRegistry, etc. These are declaration-only with no `.cpp` implementations — left intentionally.

---

## Next Steps

### Phase 51: VM API & Error Handling ✅

- [x] `global_object.cpp` refactored — delegates to builtin factories
- [x] Error hierarchy (7 types with ES2022 cause), URI encoding, queueMicrotask, globalThis
- [x] `async.cpp` — AsyncExecutionContext, AsyncFunction, AwaitHandler, MicrotaskQueue (170 lines)
- [x] `WasmStackManager.cpp` — platform stack probing, guard region (65 lines)
- [x] `OP_DEBUGGER` in VM dispatch

### Phase 52: VM Optimization ✅

- [x] `shapeId_` on Object + `nextShapeId_` counter
- [x] Shape transitions on property add/delete (IC invalidation)
- [x] `ICManager` in VM class
- [x] `OP_GET_PROPERTY` — IC fast path on shape match, slow path updates cache
- [x] `ownPropertyNames()` on Object

### Phase 53: VM Event Loop & Global Access ✅

- [x] `OP_GET_GLOBAL` — single `.find()` replacing double hash lookup
- [x] `OP_SET_GLOBAL`/`OP_DEFINE_GLOBAL` — `const std::string&` ref (no copy)
- [x] `MicrotaskQueue::instance().process()` drain after VM `run()`
- [x] Promise integration confirmed (353 lines — all/race/allSettled/any/withResolvers)
- [x] GEMINI.md updated to v1.2.0

### Phase 54: Tier 1 — Runtime API Surface ✅

- [x] Map prototype (6 methods: get/set/has/delete/clear/forEach) + constructor
- [x] Set prototype (5 methods: add/has/delete/clear/forEach) + constructor
- [x] Date prototype (25 methods: getters, UTC getters, string formatters, valueOf) + constructor + Date.now()
- [x] WeakMap prototype (4 methods: get/set/has/delete) + constructor
- [x] Getter/setter accessor invocation in `OP_GET_PROPERTY` and `OP_SET_PROPERTY`
- [x] Error stack traces in `OP_THROW` (walks heapStack\_, formats name+message+call sites)
- [x] All constructors registered in `global_object.cpp` with prototype wiring

### Phase 55: Well-Known Symbols & Iterator Protocol ✅

- [x] `well_known_symbols.hpp` — 12 reserved symbol IDs (Iterator, ToPrimitive, HasInstance, etc.)
- [x] Symbol constructor upgraded — 11 well-known symbol properties (Symbol.iterator, etc.)
- [x] `OP_GET_ITERATOR` dispatch — creates iterator state for Array/Map/Set/String
- [x] `OP_ITERATOR_NEXT` dispatch — advances iterator, returns {value, done}

### Phase 56: RegExp Engine ✅

- [x] RegExp prototype filled (test/exec/toString) — std::regex backend
- [x] RegExp constructor registered in `global_object.cpp` with prototype wiring
- [x] `String.prototype.match()` — regex-aware (accepts RegExp or string pattern)
- [x] `String.prototype.search()` — regex-aware (accepts RegExp or string pattern)

### Phase 57: Tier 2 — Framework Compatibility

- [x] Proxy / Reflect
- [x] TypedArrays + ArrayBuffer
- [x] Generator protocol (next/return/throw)
- [x] Object.freeze/seal/isSealed/isFrozen
- [x] Scope chain optimization (indexed slots)
- [x] Offset-based IC (direct slot access)
- [x] Computed goto dispatch
- [ ] Test262 compliance pass

### Phase 58: Core Gap Closure ✅

- [x] Update expressions (++/--) — prefix/postfix for local, global, member expressions
- [x] `this` expression — resolveLocal/resolveUpvalue in bytecode generator
- [x] Unicode string escapes `\uXXXX` and `\u{XXXXX}` — full UTF-8 encoding in lexer
- [x] VM `construct()` method — prototype wiring + executeCallback
- [x] Arrow function `this` capture — upvalue resolution for lexical `this`
- [x] Map SameValueZero — NaN === NaN, +0 === -0 per ES spec

### Phase 59: Pipeline & GC Hardening ✅

- [x] OP_CLOSURE upvalue emission — upvalue count + (isLocal, index) descriptors for function/arrow
- [x] ConcurrentSweepTask — std::thread parallel page sweeping with per-thread dead lists
- [x] ScriptImpl::compile() — real SourceCode→Parser→SyntaxChecker→BytecodeGenerator pipeline
- [x] ContextImpl — global object provider with setGlobal/getGlobal (no longer a stub)
- [x] VMModuleIntegration — executor->execute(module) for real module evaluation

### Phase 60: ES Feature Compilation ✅

- [x] ForInStmt AST class + compileForInStatement (OP_FOR_IN + key array iteration loop)
- [x] TemplateLiteralExpr AST class + compileTemplateLiteral (quasis/expressions with OP_ADD)
- [x] super() call in class constructor using OP_SUPER_CALL (was placeholder)
- [x] Class constructor closure upvalue count byte emission
- [x] Class method closure upvalue count byte emission

### Phase 61: VM Opcode Handlers & ES Features ✅

- [x] 8 Missing VM Opcodes implemented: `OP_INHERIT`, `OP_DEFINE_METHOD`, `OP_DEFINE_STATIC`, `OP_DEFINE_GETTER`, `OP_DEFINE_SETTER`, `OP_SUPER_CALL`, `OP_FOR_IN`
- [x] Fixed `PropertyDescriptor` memory-attribute mapping, properly invoking `Object::defineProperty`
- [x] Fixed `OP_FOR_IN` iterator array initialization strategy using `Object::keys()`
- [x] Added `SpreadElement` and `RestElement` AST classes to frontend (`ast.hpp`)
- [x] Implemented `compileSpreadElement` mapped to `OP_SPREAD` byte emission
- [x] Implemented proper argument array collection for `RestElement` mapping
- [x] Fixed `compileFunctionDeclaration`/`compileFunctionExpression`/`compileArrowFunction` for default parameter support using `OP_NIL` and `OP_STRICT_EQUAL` equality condition jump

### Phase 62: VM Opcode Completion & Stub Cleanup ✅

- [x] 11 missing VM opcode handlers: `OP_SWAP`, `OP_INCREMENT`, `OP_DECREMENT`, `OP_JUMP_IF_TRUE`, `OP_IN`, `OP_DELETE_PROPERTY`, `OP_INIT_ELEMENT`, `OP_FOR_OF`, `OP_SWITCH`, `OP_CASE`, `OP_END`
- [x] Silent `default` replaced with proper unknown-opcode error throw
- [x] Parser `for...in` detection added alongside `for...of` — produces `ForInStmt` AST nodes
- [x] `ast.cpp` filled with production `nodeTypeName()` utility (was 9-line stub)
- [x] `OP_FOR_OF` production handler: Array, String, generic Object iteration

### Phase 63: Directory Renaming & Independence Branding ✅

- [x] `b3/` → `zir/` (Zepra Intermediate Representation), all `B3*` files → `ZIR*`
- [x] `dfg/` → `zopt/` (Zepra Optimizer), all `DFG*` files → `ZOpt*`
- [x] `dfg/passes/` → `zopt/passes/`, all `DFG*` passes → `ZOpt*`
- [x] `jit/dfg/` → `jit/zopt/`
- [x] `jit/WarpBuilder.h` → `jit/ZepraTierBuilder.h`
- [x] `tests/unit/b3_tests.cpp` → `tests/unit/zir_tests.cpp`
- [x] All `#include` paths, namespace refs (`Zepra::DFG` → `Zepra::ZOpt`), and CMakeLists.txt updated
- [x] Zero WebKit/SpiderMonkey naming remaining in directory structure

### Phase 64: Runtime Stub Hardening ✅

- [x] VM: OP_AWAIT drains microtask queue on pending promises before fallback
- [x] VM: OP*YIELD yield\* delegate stores iterator in yieldedValue* for generator next()
- [x] VM: OP_SPREAD iterates strings (characters) and generic objects (own keys), throws TypeError on non-iterables
- [x] VM: OP_EXPORT stores in globals (correct for current architecture, no dedicated ModuleRecord yet)
- [x] value.cpp: `toNumber()` uses ToPrimitive valueOf→toString chain, BigInt→TypeError
- [x] value.cpp: `toObject()` boxes Number/Boolean primitives into wrapper Objects
- [x] symbol.cpp: `ObjectType::Arguments` → `ObjectType::Symbol` (proper type)
- [x] object.hpp: Added `ObjectType::Symbol` to enum
- [x] opcode.cpp: Complete `opcodeName()`, `opcodeOperandCount()`, `opcodeStackEffect()` for all 74 opcodes
- [x] bytecode_generator.cpp: Rest parameter emits `OP_CREATE_ARRAY`, export declaration emits `OP_EXPORT` for function decls

### Phase 65: Wire Public API to VM Pipeline ✅

- [x] `zepra_api.hpp`: Replaced forward declarations with `using Value = Runtime::Value` type aliases
- [x] `context_impl.hpp`: `ContextImpl` implements `Context::evaluate()` with full parse→check→compile→VM pipeline
- [x] `isolate.cpp`: `IsolateImpl::createContext()` returns real `ContextImpl` (was `nullptr`)
- [x] `script_engine.cpp`: New `ScriptEngineImpl` — `execute()`, `executeFile()`, `registerFunction()`, `setGlobal()`/`getGlobal()`
- [x] CMakeLists updated: added `script_engine.cpp`, removed `context.cpp` (now header)

### Phase 66: Promise + Complex JS ✅

- [x] Promise constructor registered in `global_object.cpp` — executor(resolve, reject), `.then()`, `.catch()`, `.finally()`
- [x] `Promise.resolve()`, `Promise.reject()`, `Promise.all()`, `Promise.race()` static methods
- [x] Verified: `??` (nullish coalescing) already implemented — parser + `OP_DUP`/`OP_JUMP_IF_NIL` bytecode
- [x] Verified: `?.` (optional chaining) already implemented — parser + MemberExpr/CallExpr optional flag

### Phase 67: Memory & GC Hardening ✅

- [x] Created `docs/zeprascript/health-report.md` — full subsystem maturity audit
- [x] Created `tools/engine_health.py` — live monitoring tool with `--watch`/`--json` modes
- [x] Wired `GCHeap` to VM via `ContextImpl` — auto-created on context creation
- [x] Verified: GC safe-points already in VM dispatch loop (maybeCollect every 1K instructions)
- [x] Verified: Write barriers already in OP_SET_PROPERTY
- [x] Verified: Generational collector (young 2MB bump-pointer, old gen malloc, promotion at age 2)

### Phase 68: Security Sandbox ✅

- [x] Verified: `Sandbox.h` (269 lines) already has `ExecutionLimits`, `SecurityPolicy`, `ResourceMonitor`, `SandboxConfig`, `SecurityError`
- [x] Verified: `SecurityAudit.h` (449 lines) already has `ObjectTracker` (UAF detection), `RefTracker`, `ExceptionStateValidator`
- [x] Wired `SandboxConfig::browser()` + `ResourceMonitor` into `ContextImpl`
- [x] VM now receives `ResourceMonitor` via `setSandbox()` — enforces heap/timeout/stack/instruction limits
- [x] Browser defaults: 512MB heap, 30s timeout, 1B instructions, 10K call stack depth

### Phase 69: Crash Recovery & Process Architecture ✅

- [x] Created `CrashHandler.h` — `CrashContext`, `CrashDumpWriter`, `CrashHandler`, `ExecutionWatchdog`
- [x] Created `crash_handler.cpp` — POSIX signal handlers (SIGSEGV/SIGBUS/SIGABRT/SIGFPE)
- [x] Crash dump writer: async-signal-safe (raw fd writes), captures VM IP, stack depth, heap, backtrace
- [x] `ExecutionWatchdog`: progress-counter-based timeout detection, sets VM termination flag on hang
- [x] Added to CMakeLists, build verified

### Phase 70: JIT Maturation ✅

- [x] Verified: JIT subsystem has 17 files — baseline_jit, macro_assembler (x86-64), deoptimizer, InlineCache, type_profiler, OSR, GraphColoringRegAlloc, ZepraTierBuilder
- [x] Verified: `ICManager` already in VM (property access inline caching)
- [x] Wired `JITProfiler` into VM: `recordCall()` on every `OP_CALL` for hot function detection
- [x] Hot thresholds: 100 calls → baseline candidate, 1000 calls → optimization candidate

### Phase 71: WASM Completeness ✅

- [x] Deep audit: 50 files, 27K lines, only 18 stubs (mostly in 310K baseline compiler)
- [x] Verified: GC barriers (ZWasmGCBarriers — card table + SATB), guard pages (2MB), bounds checking
- [x] Added WASM ObjectType enum values: `Namespace`, `WasmModule`, `WasmInstance`, `WasmMemory`, `WasmTable`, `WasmGlobal`
- [x] Registered `WebAssembly` global in `global_object.cpp`: `validate()`, `compile()`, `instantiate()`, `Memory`, `Table`, `Global`, `CompileError`, `LinkError`, `RuntimeError`
- [x] Build verified

### Phase 72: Browser Integration & Background Tasks ✅

- [x] Verified: `console` already registered (Builtins::Console), `queueMicrotask` registered
- [x] Verified: browser/ has 37 files (11K lines) — fetch (31K), IndexedDB (25K), workers, WebSocket, DOM, events, performance, URL, storage, video bindings
- [x] Registered `setTimeout`, `setInterval`, `clearTimeout`, `clearInterval`
- [x] Registered `atob`, `btoa`, `structuredClone`
- [x] Build verified

### Phase 73: GC Expansion (In Progress)

Target: 80K GC lines | Current: ~50K | 120+ files across heap/, runtime/, test/

#### Batches 1–15 ✅ — Foundation

- Heap infrastructure: gc_heap, gc_pipeline, concurrent_gc, marking, sweeping, compacting
- Generational GC, remembered sets, concurrent marker/sweeper, parallel scanner
- Shape table, property storage, object layout, slab allocator, region allocator
- GC debug, stress, benchmark, verification, integration tests
- Weak processing, finalizer engine, ephemeron table, JIT-GC bridge, code space manager

#### Batch 16 ✅ — Heap Spaces + Runtime Integration

- gc_nursery (semi-space Cheney scavenge), gc_old_space (segregated free lists)
- gc_large_object (per-object mmap), gc_sweeper (page walker)
- gc_object_visitor, gc_runtime_bridge, gc_write_barrier (runtime side)
- gc_nursery_test, gc_barrier_test

#### Batch 17 ✅ — Scheduling + Weak Processing

- gc_scheduler (budget + occupancy + idle + pressure), gc_telemetry (ring buffer + histogram)
- gc_weak_processor (ephemeron fixpoint), gc_finalizer_queue (FIFO drain)
- gc_incremental (tri-color marking), gc_lifecycle_hooks (runtime hooks)
- gc_weak_test, gc_scheduler_test

#### Batch 18 ✅ — TLAB + Snapshot + JIT Barriers

- gc_tlab (thread-local allocation), gc_heap_snapshot_serializer (V8 format)
- gc_barrier_codegen (x86-64/AArch64 stubs), gc_region_metadata
- gc_safepoint_hooks (runtime), gc_tlab_test, gc_snapshot_test

#### Batch 19 ✅ — Evacuation + Pretenuring

- gc_evacuation (forwarding table + 3-phase), gc_pretenuring (allocation-site feedback)
- gc_marking_bitmap (atomic 1-bit/granule), gc_stack_scanner (conservative)
- gc_stats_api (runtime), gc_evacuation_test, gc_pretenure_test

#### Batch 20 ✅ — Card Table + Pinning + Age Tracking

- gc_card_table (512B cards), gc_object_pinning (ref-counted)
- gc_age_table (adaptive tenuring), gc_heap_iterator, gc_sweep_worklist
- gc_pressure_api (runtime), gc_card_table_test, gc_age_table_test

#### Batch 21+ — In Progress

---
