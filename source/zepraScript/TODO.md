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
- [ ] Offset-based IC (direct slot access)
- [x] Computed goto dispatch
- [ ] Test262 compliance pass

### Phase 58: Core Gap Closure ✅

- [x] Update expressions (++/--) — prefix/postfix for local, global, member expressions
- [x] `this` expression — resolveLocal/resolveUpvalue in bytecode generator
- [x] Unicode string escapes `\uXXXX` and `\u{XXXXX}` — full UTF-8 encoding in lexer
- [x] VM `construct()` method — prototype wiring + executeCallback
- [x] Arrow function `this` capture — upvalue resolution for lexical `this`
- [x] Map SameValueZero — NaN === NaN, +0 === -0 per ES spec
