# GEMINI.md - AI Codebase Maintenance Guide

**Project:** ZebraScript JavaScript Engine  
**Purpose:** Guide for AI assistants (Gemini, Claude, GPT, etc.) to maintain, edit, and extend this codebase  
**Last Updated:** 2024-12-07

---

## 🤖 About This Document

This document helps AI models understand how to work with the ZebraScript codebase. It provides context, architecture overview, coding standards, and instructions for common maintenance tasks.

---

## 📋 Table of Contents

1. [Project Overview](#project-overview)
2. [Architecture Summary](#architecture-summary)
3. [Directory Structure](#directory-structure)
4. [Core Design Principles](#core-design-principles)
5. **Performance matters**
   - Profile before optimizing
   - Use inline caching for hot paths
   - Minimize allocations in critical code
   - Consider cache locality

6. **Security first**
   - Validate all inputs
   - Check for integer overflow
   - Use GC handles to prevent use-after-free
   - Sanitize user strings in errors

### When Adding Features

**Checklist:**
- [ ] Create header file with forward declarations
- [ ] Implement in corresponding .cpp file
- [ ] Add to CMakeLists.txt if new files
- [ ] Write unit tests
- [ ] Write integration tests (if needed)
- [ ] Update documentation
- [ ] Run test suite: `make test`
- [ ] Run benchmarks: `make benchmark`
- [ ] Update this GEMINI.md if architecture changes

### When Fixing Bugs

1. **Reproduce the bug**
   - Write a failing test first
   - Understand the root cause

2. **Fix minimally**
   - Change only what's necessary
   - Don't refactor while fixing bugs

3. **Verify the fix**
   - Ensure the test passes
   - Check for regressions
   - Run full test suite

4. **Document the fix**
   - Add comment explaining why fix is needed
   - Update changelog if user-facing

### When Refactoring

1. **Have tests**
   - Never refactor without good test coverage
   - Add tests if missing

2. **Refactor incrementally**
   - Small, focused changes
   - Keep tests passing at each step

3. **Maintain API compatibility**
   - Don't break public APIs
   - Deprecate before removing

4. **Update documentation**
   - Keep docs in sync with code
   - Update examples if needed

### Code Review Guidelines

When reviewing or suggesting code changes:

1. **Check architectural compliance**
   - Does it maintain separation of concerns?
   - Are dependencies correct?
   - Is it in the right module?

2. **Check code quality**
   - Follows naming conventions?
   - Has appropriate comments?
   - Error handling present?
   - No memory leaks?

3. **Check testing**
   - Has unit tests?
   - Has integration tests if needed?
   - Tests cover edge cases?

4. **Check performance**
   - Any obvious performance issues?
   - Allocations in hot paths?
   - Could use caching/optimization?

5. **Check security**
   - Input validation present?
   - No buffer overflows?
   - No integer overflows?
   - No use-after-free?

---

## Quick Reference Commands

### Build Commands

```bash
# Configure build
cmake -B build -DCMAKE_BUILD_TYPE=Release \
      -DZEPRA_BUILD_DEVTOOLS=ON \
      -DZEPRA_BUILD_CDP_EXTENSION=OFF

# Build all targets
cmake --build build -j$(nproc)

# Build specific target
cmake --build build --target zepra-core

# Install
cmake --install build --prefix /usr/local
```

### Testing Commands

```bash
# Run all tests
cd build && ctest

# Run specific test suite
./build/tests/unit/value_tests

# Run with verbose output
ctest --verbose

# Run test262 compliance tests
cd tests/test262 && ./run-tests.sh
```

### Debugging Commands

```bash
# Run with debugger
gdb ./build/zepra-repl

# Run with sanitizers
cmake -B build -DCMAKE_BUILD_TYPE=Debug \
      -DENABLE_ASAN=ON -DENABLE_UBSAN=ON
cmake --build build
./build/zepra-repl

# Memory leak detection
valgrind --leak-check=full ./build/zepra-repl script.js
```

### Profiling Commands

```bash
# CPU profiling with perf
perf record -g ./build/zepra-repl script.js
perf report

# Heap profiling with massif
valgrind --tool=massif ./build/zepra-repl script.js
ms_print massif.out.12345

# Run benchmarks
./build/benchmarks/bench_execution
```

### Code Quality Commands

```bash
# Format code
clang-format -i src/**/*.cpp include/**/*.hpp

# Static analysis
clang-tidy src/**/*.cpp -- -Iinclude

# Check includes
iwyu ./src/runtime/vm.cpp
```

---

## Common Pitfalls to Avoid

### 1. Including DevTools in Core

```cpp
// ❌ NEVER do this in src/
#include <zepra_devtools/debugger.hpp>
#include <zeprascript/cdp/cdp_server.hpp>

// ✅ Only include core headers
#include <zeprascript/debug/debug_api.hpp>
```

### 2. Forgetting GC Handles

```cpp
// ❌ BAD: Object might be collected
Object* obj = heap->allocate<Object>();
someFunction();  // Might trigger GC
obj->setProperty(...);  // Crash!

// ✅ GOOD: Use handle
Handle<Object> obj = heap->allocate<Object>();
someFunction();  // GC can run safely
obj->setProperty(...);  // obj is protected
```

### 3. Not Checking Array Bounds

```cpp
// ❌ BAD: No bounds check
Value get(size_t index) {
    return elements_[index];  // Undefined behavior if out of bounds
}

// ✅ GOOD: Always check
Value get(size_t index) {
    if (index >= elements_.size()) {
        return Value::undefined();
    }
    return elements_[index];
}
```

### 4. Ignoring Integer Overflow

```cpp
// ❌ BAD: Can overflow
int32_t result = a + b;

// ✅ GOOD: Check for overflow
if (a > INT32_MAX - b) {
    throw RangeError("Arithmetic overflow");
}
int32_t result = a + b;
```

### 5. Memory Leaks in Exceptions

```cpp
// ❌ BAD: Leaks if exception thrown
void process() {
    Resource* res = new Resource();
    riskyOperation();  // Might throw
    delete res;  // Never reached if exception thrown
}

// ✅ GOOD: Use RAII
void process() {
    std::unique_ptr<Resource> res = std::make_unique<Resource>();
    riskyOperation();  // Automatically cleaned up on exception
}
```

### 6. Inefficient String Operations

```cpp
// ❌ BAD: Reallocates every iteration
std::string result;
for (int i = 0; i < 10000; i++) {
    result += std::to_string(i) + ",";
}

// ✅ GOOD: Use StringBuilder or reserve
StringBuilder builder;
for (int i = 0; i < 10000; i++) {
    builder.append(std::to_string(i));
    builder.append(",");
}
std::string result = builder.toString();
```

### 7. Not Initializing Variables

```cpp
// ❌ BAD: Uninitialized
int count;
if (condition) {
    count = 10;
}
return count;  // Undefined if condition is false

// ✅ GOOD: Always initialize
int count = 0;
if (condition) {
    count = 10;
}
return count;
```

---

## Module Dependencies Graph

```
┌─────────────────┐
│   zepra-core    │  (Core engine - no dependencies on other modules)
└────────┬────────┘
         │
         │ uses
         ↓
┌─────────────────┐
│  debug/         │  (Native debug API - part of core)
│  debug_api.hpp  │
└────────┬────────┘
         │
         ├─────────────────────┐
         │                     │
         │ used by             │ used by
         ↓                     ↓
┌─────────────────┐   ┌────────────────┐
│ zepra-devtools  │   │ cdp-extension  │  (Both optional)
│ (Native UI)     │   │ (CDP bridge)   │
└─────────────────┘   └────────────────┘
```

**Rule:** Arrows only point downward. Core never depends on tools above it.

---

## Emergency Procedures

### If Build is Broken

1. **Clean build directory**
   ```bash
   rm -rf build
   cmake -B build
   cmake --build build
   ```

2. **Check for missing dependencies**
   ```bash
   cmake -B build 2>&1 | grep "Could not find"
   ```

3. **Verify compiler version**
   ```bash
   g++ --version  # Needs GCC 11+ or Clang 14+
   ```

### If Tests Are Failing

1. **Run single test**
   ```bash
   ./build/tests/unit/value_tests --gtest_filter=ValueTests.NumberCreation
   ```

2. **Run with debugger**
   ```bash
   gdb --args ./build/tests/unit/value_tests --gtest_filter=ValueTests.NumberCreation
   ```

3. **Check test262 failures**
   ```bash
   cd tests/test262
   ./run-tests.sh --verbose --filter=failing
   ```

### If Performance Regresses

1. **Run benchmarks**
   ```bash
   ./build/benchmarks/bench_execution --benchmark_out=results.json
   ```

2. **Compare with baseline**
   ```bash
   python tools/compare_benchmarks.py baseline.json results.json
   ```

3. **Profile hot spots**
   ```bash
   perf record -g ./build/zepra-repl slow_script.js
   perf report
   ```

---

## Contact and Resources

### Documentation
- Architecture: `docs/architecture.md`
- Bytecode Spec: `docs/bytecode-spec.md`
- JIT Design: `docs/jit-tiers.md`
- GC Algorithm: `docs/gc-algorithm.md`
- Debug Protocol: `docs/native-debug-protocol.md`

### External Resources
- ECMAScript Spec: https://tc39.es/ecma262/
- Test262: https://github.com/tc39/test262
- JavaScriptCore: https://webkit.org/blog/tag/javascriptcore/
- V8 Design: https://v8.dev/docs

### Project Structure
- Core Engine: `src/` and `include/zeprascript/`
- DevTools: `zepra-devtools/`
- CDP Extension: `cdp-extension/`
- Tests: `tests/`
- Docs: `docs/`
- Tools: `tools/`

---

## Revision History

| Version | Date | Changes |
|---------|------|---------|
| 1.0.0 | 2024-12-07 | Initial version |

---

**Remember:** This codebase prioritizes performance, security, and maintainability. When in doubt, look at existing code for patterns and ask questions in comments.. [Coding Standards](#coding-standards)
6. [Common Maintenance Tasks](#common-maintenance-tasks)
7. [Testing Guidelines](#testing-guidelines)
8. [Performance Considerations](#performance-considerations)
9. [Security Guidelines](#security-guidelines)
10. [AI Assistant Instructions](#ai-assistant-instructions)

---

## 1. Project Overview

### What is ZebraScript?

ZebraScript is a high-performance JavaScript engine designed for the ZepraBrowser web browser. It aims to provide JavaScriptCore/V8-level performance and features while maintaining complete independence from third-party vendors.

### Key Goals

- **Performance**: Multi-tier JIT compilation (Baseline → DFG → FTL)
- **Standards Compliance**: Full ECMAScript 2024+ support
- **Browser Integration**: Native DOM, Fetch, Events, Workers
- **Independence**: Own DevTools, own protocol, no vendor lock-in
- **Modularity**: Clean separation of concerns

### Technology Stack

- **Language**: C++20
- **Build System**: CMake 3.20+
- **Testing**: Google Test
- **JIT**: Custom assembler for x86/x64/ARM/ARM64
- **GC**: Generational, incremental, concurrent
- **DevTools UI**: Qt6 (optional: GTK, ImGui)

---

## 2. Architecture Summary

### Three-Layer Design

```
┌─────────────────────────────────────┐
│   Zepra DevTools (Native C++ UI)    │  ← Primary debugging tool
│   • Direct API calls                │
│   • Zero protocol overhead          │
└─────────────────┬───────────────────┘
                  │
┌─────────────────▼───────────────────┐
│   Native Debug API                  │  ← Always present
│   • Breakpoints, call stack         │
│   • Variable inspection             │
└─────────────────┬───────────────────┘
                  │
┌─────────────────▼───────────────────┐
│   ZebraScript Core Engine           │  ← Never depends on debug tools
│   • VM, JIT, GC, Runtime            │
└─────────────────────────────────────┘

┌─────────────────────────────────────┐
│   CDP Extension (Optional)          │  ← Can be deleted
│   • Chrome DevTools compatibility   │
│   • Translates CDP ↔ Native API     │
└─────────────────────────────────────┘
```

### Execution Pipeline

```
Source Code
    ↓
Lexer → Tokens
    ↓
Parser → AST
    ↓
Compiler → Bytecode
    ↓
Interpreter (Tier 0) + Type Profiler
    ↓ (hot paths)
Baseline JIT (Tier 1)
    ↓ (very hot paths)
DFG JIT (Tier 2) + Speculative Optimizer
    ↓ (critical hot paths)
FTL JIT (Tier 3) + LLVM-like Backend
    ↓
Native Machine Code
```

---

## 3. Directory Structure

### Core Components (Required)

```
src/
├── frontend/          # Lexer, Parser, AST
├── compiler/          # Bytecode compiler, optimizer
├── runtime/           # VM, GC, Object model
├── builtins/          # Array, String, Math, etc.
├── jit/               # Multi-tier JIT compiler
├── gc/                # Garbage collector
├── memory/            # Memory management
├── host/              # C++ ↔ JS bridge
├── browser/           # DOM, Fetch, Events
├── debug/             # Native debug API
├── bytecode/          # Bytecode generation
├── interpreter/       # Bytecode interpreter
└── exception/         # Exception handling
```

### Optional Components

```
zepra-devtools/        # Native DevTools UI (Primary)
cdp-extension/         # CDP compatibility (Optional)
tests/                 # Unit and integration tests
benchmarks/            # Performance benchmarks
tools/                 # CLI tools, REPL
docs/                  # Documentation
```

---

## 4. Core Design Principles

### 1. Independence from Debug Tools

**CRITICAL:** The core engine (`src/`) must NEVER depend on debugging tools.

```cpp
// ❌ WRONG - Core engine depending on DevTools
#include <zepra_devtools/debugger.hpp>  // Never in src/

// ✅ CORRECT - Core engine exposes debug API
#include <zeprascript/debug/debug_api.hpp>  // Always fine
```

### 2. Zero Debug Overhead in Production

```cpp
// Debug hooks should have zero cost when not used
class DebugAPI {
    // Use function pointers that default to nullptr
    PausedCallback paused_callback_ = nullptr;
    
    void notifyPaused(const StackFrame& frame) {
        if (paused_callback_) {  // Zero overhead if not set
            paused_callback_(frame);
        }
    }
};
```

### 3. Modular Architecture

Each module should be independently compilable and testable:

- `libzepra-core.so` - Core engine (required)
- `libzepra-browser.so` - Browser APIs (optional)
- `libzepra-devtools.so` - DevTools UI (optional)
- `libzepra-cdp.so` - CDP extension (optional)

### 4. Performance First

- JIT compilation for hot code paths
- Inline caching for property access
- Hidden classes for object layout optimization
- Generational GC to reduce pause times
- SIMD for string operations

### 5. Memory Safety

- RAII for resource management
- Smart pointers for ownership
- GC handles for JS objects
- Write barriers for generational GC

---

## 5. Coding Standards

### C++ Style Guide

#### File Organization

```cpp
// header.hpp
#pragma once

#include <vector>  // Standard library first
#include <string>

#include "zeprascript/config.hpp"  // Project headers second
#include "zeprascript/runtime/value.hpp"

namespace Zepra::Runtime {

class MyClass {
public:
    MyClass();
    ~MyClass();
    
    void publicMethod();
    
private:
    void privateMethod();
    
    int member_variable_;  // Use trailing underscore
};

} // namespace Zepra::Runtime
```

#### Naming Conventions

```cpp
// Classes: PascalCase
class VirtualMachine { };

// Functions/Methods: camelCase
void executeFunction();

// Variables: snake_case
int local_variable;
std::string file_name;

// Member variables: snake_case with trailing underscore
class Example {
    int member_var_;
    std::string name_;
};

// Constants: SCREAMING_SNAKE_CASE
constexpr int MAX_STACK_SIZE = 1024;

// Namespaces: PascalCase
namespace Zepra::Runtime { }
```

#### Modern C++ Features

```cpp
// Use auto for complex types
auto result = vm->execute(code);

// Use range-based for loops
for (const auto& item : collection) { }

// Use smart pointers
std::unique_ptr<VM> vm = VM::create();
std::shared_ptr<Context> ctx;

// Use std::optional for nullable values
std::optional<Value> getValue(const std::string& key);

// Use structured bindings
auto [success, result] = tryParse(input);

// Use if-init statements
if (auto value = getValue(key); value) {
    // use value
}
```

#### Error Handling

```cpp
// Use exceptions for exceptional cases
class RuntimeException : public std::runtime_error {
public:
    explicit RuntimeException(const std::string& msg)
        : std::runtime_error(msg) {}
};

// Use std::optional for expected failures
std::optional<int> parseInteger(const std::string& str) {
    try {
        return std::stoi(str);
    } catch (...) {
        return std::nullopt;
    }
}

// Use Result<T, E> for operations that can fail
template<typename T, typename E>
class Result {
    // Implementation
};
```

### Header File Best Practices

```cpp
// Always use #pragma once
#pragma once

// Include guards as fallback (optional)
#ifndef ZEPRA_RUNTIME_VALUE_HPP
#define ZEPRA_RUNTIME_VALUE_HPP

// Forward declarations to reduce includes
namespace Zepra::Runtime {
    class Object;
    class Function;
}

// Minimize includes in headers
class Value {
    // Use pointers/references for forward-declared types
    Object* toObject();
};

#endif // ZEPRA_RUNTIME_VALUE_HPP
```

---

## 6. Common Maintenance Tasks

### Adding a New Builtin Function

**Example: Adding `Array.prototype.findLast()`**

1. **Update header** (`include/zeprascript/builtins/array.hpp`):

```cpp
namespace Zepra::Builtins {

class ArrayPrototype {
public:
    // Existing methods...
    
    // Add new method
    static Value findLast(const FunctionCallInfo& info);
};

} // namespace Zepra::Builtins
```

2. **Implement method** (`src/builtins/array.cpp`):

```cpp
Value ArrayPrototype::findLast(const FunctionCallInfo& info) {
    // Get 'this' value
    Value thisValue = info.thisValue();
    
    // Convert to object
    Object* obj = thisValue.toObject();
    if (!obj) {
        throw TypeError("Array.prototype.findLast called on null or undefined");
    }
    
    // Get callback function
    if (info.argumentCount() < 1 || !info.argument(0).isFunction()) {
        throw TypeError("Array.prototype.findLast requires a function");
    }
    
    Function* callback = info.argument(0).asFunction();
    Value thisArg = info.argumentCount() > 1 ? info.argument(1) : Value::undefined();
    
    // Get array length
    int64_t length = obj->getLength();
    
    // Iterate backwards
    for (int64_t i = length - 1; i >= 0; --i) {
        Value element = obj->get(i);
        
        // Call callback(element, index, array)
        std::vector<Value> args = {element, Value(i), thisValue};
        Value result = callback->call(thisArg, args);
        
        // Check result
        if (result.toBoolean()) {
            return element;
        }
    }
    
    return Value::undefined();
}
```

3. **Register method** (`src/runtime/global_object.cpp`):

```cpp
void GlobalObject::initializeArrayPrototype() {
    // Existing methods...
    
    // Register new method
    array_prototype_->defineOwnProperty(
        "findLast",
        Value::createNativeFunction(ArrayPrototype::findLast),
        PropertyAttribute::Writable | PropertyAttribute::Configurable
    );
}
```

4. **Add tests** (`tests/unit/array_tests.cpp`):

```cpp
TEST(ArrayTests, FindLast) {
    auto vm = VM::create();
    auto result = vm->execute(R"(
        const arr = [1, 2, 3, 4, 5];
        const found = arr.findLast(x => x > 3);
        found; // Should be 5
    )");
    
    EXPECT_EQ(result.asNumber(), 5);
}

TEST(ArrayTests, FindLastNotFound) {
    auto vm = VM::create();
    auto result = vm->execute(R"(
        const arr = [1, 2, 3];
        const found = arr.findLast(x => x > 10);
        found; // Should be undefined
    )");
    
    EXPECT_TRUE(result.isUndefined());
}
```

### Adding a New Bytecode Instruction

**Example: Adding `OP_NULLISH_COALESCE` for `??` operator**

1. **Define opcode** (`include/zeprascript/bytecode/opcode.hpp`):

```cpp
enum class Opcode : uint8_t {
    // Existing opcodes...
    
    OP_NULLISH_COALESCE,  // a ?? b - use b if a is null/undefined
    
    // Continue with other opcodes...
};
```

2. **Implement bytecode generation** (`src/compiler/bytecode_generator.cpp`):

```cpp
void BytecodeGenerator::visitNullishCoalesceExpression(NullishCoalesceExpression* expr) {
    // Evaluate left side
    expr->left()->accept(this);
    
    // Duplicate value for checking
    emit(Opcode::OP_DUP);
    
    // Check if null or undefined
    emit(Opcode::OP_IS_NULLISH);
    
    // If not nullish, jump over right side
    size_t skipLabel = emitJump(Opcode::OP_JUMP_IF_FALSE);
    
    // Pop the duplicated value
    emit(Opcode::OP_POP);
    
    // Evaluate right side
    expr->right()->accept(this);
    
    // Patch jump
    patchJump(skipLabel);
}
```

3. **Implement interpreter** (`src/interpreter/interpreter.cpp`):

```cpp
void Interpreter::execute() {
    while (true) {
        Opcode op = readOpcode();
        
        switch (op) {
            // Existing cases...
            
            case Opcode::OP_NULLISH_COALESCE: {
                Value left = pop();
                if (left.isNull() || left.isUndefined()) {
                    Value right = pop();
                    push(right);
                } else {
                    push(left);
                }
                break;
            }
            
            // Other cases...
        }
    }
}
```

4. **Add JIT support** (`src/jit/baseline_jit.cpp`):

```cpp
void BaselineJIT::compileNullishCoalesce() {
    // Pop left value from stack
    popToRegister(rax);
    
    // Check if null (0x02 in tagged pointer)
    cmp(rax, 0x02);
    je(useRightSide);
    
    // Check if undefined (0x06 in tagged pointer)
    cmp(rax, 0x06);
    je(useRightSide);
    
    // Left side is not nullish, use it
    pushRegister(rax);
    jmp(done);
    
    // Left side is nullish, use right side
    Label useRightSide = defineLabel();
    popToRegister(rax);  // Get right value
    pushRegister(rax);
    
    Label done = defineLabel();
}
```

5. **Add tests** (`tests/unit/nullish_coalesce_tests.cpp`):

```cpp
TEST(NullishCoalesceTests, WithNull) {
    auto vm = VM::create();
    auto result = vm->execute("null ?? 42");
    EXPECT_EQ(result.asNumber(), 42);
}

TEST(NullishCoalesceTests, WithUndefined) {
    auto vm = VM::create();
    auto result = vm->execute("undefined ?? 'default'");
    EXPECT_EQ(result.asString(), "default");
}

TEST(NullishCoalesceTests, WithValue) {
    auto vm = VM::create();
    auto result = vm->execute("0 ?? 42");
    EXPECT_EQ(result.asNumber(), 0);  // 0 is not nullish
}
```

### Adding a Browser API

**Example: Adding `navigator.clipboard` API**

1. **Create header** (`include/zeprascript/browser/clipboard_api.hpp`):

```cpp
#pragma once

#include "zeprascript/runtime/value.hpp"
#include "zeprascript/runtime/promise.hpp"

namespace Zepra::Browser {

class ClipboardAPI {
public:
    explicit ClipboardAPI(VM* vm);
    
    // Clipboard.writeText(text)
    Promise* writeText(const std::string& text);
    
    // Clipboard.readText()
    Promise* readText();
    
    // Clipboard.write(items)
    Promise* write(const std::vector<ClipboardItem>& items);
    
    // Clipboard.read()
    Promise* read();
    
private:
    VM* vm_;
    
    // Platform-specific implementations
    void writeTextToClipboard(const std::string& text);
    std::string readTextFromClipboard();
};

} // namespace Zepra::Browser
```

2. **Implement** (`src/browser/clipboard_api.cpp`):

```cpp
#include "zeprascript/browser/clipboard_api.hpp"
#include "zeprascript/runtime/promise.hpp"

namespace Zepra::Browser {

ClipboardAPI::ClipboardAPI(VM* vm) : vm_(vm) {}

Promise* ClipboardAPI::writeText(const std::string& text) {
    auto promise = vm_->createPromise();
    
    // Perform async clipboard write
    vm_->scheduleTask([this, promise, text]() {
        try {
            writeTextToClipboard(text);
            promise->resolve(Value::undefined());
        } catch (const std::exception& e) {
            promise->reject(Value::createError(e.what()));
        }
    });
    
    return promise;
}

Promise* ClipboardAPI::readText() {
    auto promise = vm_->createPromise();
    
    // Perform async clipboard read
    vm_->scheduleTask([this, promise]() {
        try {
            std::string text = readTextFromClipboard();
            promise->resolve(Value::createString(text));
        } catch (const std::exception& e) {
            promise->reject(Value::createError(e.what()));
        }
    });
    
    return promise;
}

// Platform-specific implementations
#ifdef _WIN32
void ClipboardAPI::writeTextToClipboard(const std::string& text) {
    // Windows implementation
    if (!OpenClipboard(nullptr)) {
        throw std::runtime_error("Failed to open clipboard");
    }
    
    EmptyClipboard();
    
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
    memcpy(GlobalLock(hMem), text.c_str(), text.size() + 1);
    GlobalUnlock(hMem);
    
    SetClipboardData(CF_TEXT, hMem);
    CloseClipboard();
}
#elif defined(__linux__)
void ClipboardAPI::writeTextToClipboard(const std::string& text) {
    // Linux X11/Wayland implementation
    // Use GTK or Qt clipboard API
}
#elif defined(__APPLE__)
void ClipboardAPI::writeTextToClipboard(const std::string& text) {
    // macOS implementation
    NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
    [pasteboard clearContents];
    [pasteboard setString:@(text.c_str()) forType:NSPasteboardTypeString];
}
#endif

} // namespace Zepra::Browser
```

3. **Register in Navigator** (`src/browser/window_object.cpp`):

```cpp
void WindowObject::initializeNavigator() {
    auto navigator = Object::create(vm_);
    
    // Existing properties...
    
    // Add clipboard API
    auto clipboard = Object::create(vm_);
    clipboard_api_ = std::make_unique<ClipboardAPI>(vm_);
    
    clipboard->defineOwnProperty(
        "writeText",
        Value::createNativeFunction([this](const FunctionCallInfo& info) {
            if (info.argumentCount() < 1) {
                throw TypeError("writeText requires a string argument");
            }
            std::string text = info.argument(0).toString();
            return Value::createPromise(clipboard_api_->writeText(text));
        })
    );
    
    clipboard->defineOwnProperty(
        "readText",
        Value::createNativeFunction([this](const FunctionCallInfo& info) {
            return Value::createPromise(clipboard_api_->readText());
        })
    );
    
    navigator->defineOwnProperty("clipboard", Value::createObject(clipboard));
}
```

4. **Add tests** (`tests/integration/clipboard_tests.cpp`):

```cpp
TEST(ClipboardTests, WriteText) {
    auto vm = VM::create();
    
    auto result = vm->execute(R"(
        (async () => {
            await navigator.clipboard.writeText('Hello World');
            return 'success';
        })();
    )");
    
    EXPECT_TRUE(result.isPromise());
    auto value = result.asPromise()->awaitResult();
    EXPECT_EQ(value.asString(), "success");
}

TEST(ClipboardTests, ReadText) {
    auto vm = VM::create();
    
    // Pre-populate clipboard
    navigator.clipboard.writeTextSync("Test Data");
    
    auto result = vm->execute(R"(
        (async () => {
            return await navigator.clipboard.readText();
        })();
    )");
    
    auto value = result.asPromise()->awaitResult();
    EXPECT_EQ(value.asString(), "Test Data");
}
```

### Optimizing Hot Code Paths

**Example: Optimizing array access with inline caching**

1. **Identify hot path** (use profiler):

```cpp
// Hot path: array element access
Value Array::get(int64_t index) {
    // This is called millions of times per second
    return elements_[index];
}
```

2. **Add inline cache**:

```cpp
struct PropertyCache {
    Structure* structure;  // Object shape
    size_t offset;         // Property offset
    
    bool isValid(Structure* current) const {
        return structure == current;
    }
};

Value Array::get(int64_t index, PropertyCache* cache) {
    // Fast path: cache hit
    if (cache && cache->isValid(structure_)) {
        return *reinterpret_cast<Value*>(
            reinterpret_cast<uint8_t*>(this) + cache->offset
        );
    }
    
    // Slow path: compute and update cache
    size_t offset = structure_->getPropertyOffset(index);
    if (cache) {
        cache->structure = structure_;
        cache->offset = offset;
    }
    
    return *reinterpret_cast<Value*>(
        reinterpret_cast<uint8_t*>(this) + offset
    );
}
```

3. **Use in bytecode interpreter**:

```cpp
case Opcode::OP_GET_ELEMENT: {
    Value array = pop();
    Value index = pop();
    
    // Check cache
    PropertyCache* cache = getCurrentCache();
    Value result = array.asObject()->get(index.asNumber(), cache);
    
    push(result);
    break;
}
```

4. **Benchmark**:

```cpp
BENCHMARK(ArrayAccess_WithCache) {
    auto vm = VM::create();
    vm->execute(R"(
        const arr = [1, 2, 3, 4, 5];
        for (let i = 0; i < 1000000; i++) {
            arr[0];  // Should hit cache every time
        }
    )");
}
```

---

## 7. Testing Guidelines

### Unit Tests

```cpp
// tests/unit/value_tests.cpp
#include <gtest/gtest.h>
#include "zeprascript/runtime/value.hpp"

TEST(ValueTests, NumberCreation) {
    Value v = Value::createNumber(42.0);
    EXPECT_TRUE(v.isNumber());
    EXPECT_EQ(v.asNumber(), 42.0);
}

TEST(ValueTests, StringCreation) {
    Value v = Value::createString("hello");
    EXPECT_TRUE(v.isString());
    EXPECT_EQ(v.asString(), "hello");
}

TEST(ValueTests, TypeConversion) {
    Value num = Value::createNumber(123);
    EXPECT_EQ(num.toString(), "123");
    
    Value str = Value::createString("456");
    EXPECT_EQ(str.toNumber(), 456.0);
}
```

### Integration Tests

```cpp
// tests/integration/promise_tests.cpp
TEST(PromiseTests, BasicResolve) {
    auto vm = VM::create();
    
    auto result = vm->execute(R"(
        let resolved = false;
        const promise = new Promise(resolve => {
            resolve(42);
        });
        
        promise.then(value => {
            resolved = value;
        });
        
        // Wait for microtask queue
        await promise;
        resolved;
    )");
    
    EXPECT_EQ(result.asNumber(), 42);
}
```

### Performance Tests

```cpp
// benchmarks/bench_array.cpp
#include <benchmark/benchmark.h>
#include "zeprascript/script_engine.hpp"

static void BM_ArrayPush(benchmark::State& state) {
    auto vm = VM::create();
    
    for (auto _ : state) {
        vm->execute(R"(
            const arr = [];
            for (let i = 0; i < 10000; i++) {
                arr.push(i);
            }
        )");
    }
}
BENCHMARK(BM_ArrayPush);
```

### Test262 Compliance

```bash
# Run official ECMAScript conformance tests
cd tests/test262
./run-tests.sh --engine=zepra --parallel=8
```

---

## 8. Performance Considerations

### Memory Allocation

```cpp
// ❌ BAD: Frequent allocations
for (int i = 0; i < 1000000; i++) {
    std::vector<int> v;  // Allocated every iteration
    v.push_back(i);
}

// ✅ GOOD: Reuse allocations
std::vector<int> v;
v.reserve(1000000);  // Pre-allocate
for (int i = 0; i < 1000000; i++) {
    v.push_back(i);
}

// ✅ BETTER: Use arena allocator
ArenaAllocator arena(1024 * 1024);  // 1MB pool
for (int i = 0; i < 1000000; i++) {
    int* p = arena.allocate<int>();
    *p = i;
}
// All freed at once when arena is destroyed
```

### String Operations

```cpp
// ❌ BAD: String concatenation in loop
std::string result;
for (const auto& str : strings) {
    result += str;  // Reallocates every time
}

// ✅ GOOD: Use StringBuilder
StringBuilder builder;
for (const auto& str : strings) {
    builder.append(str);
}
std::string result = builder.toString();

// ✅ BETTER: Reserve capacity
std::string result;
size_t total_size = 0;
for (const auto& str : strings) {
    total_size += str.size();
}
result.reserve(total_size);
for (const auto& str : strings) {
    result += str;
}
```

### Cache Locality

```cpp
// ❌ BAD: Poor cache locality
struct Object {
    std::string name;          // 32 bytes
    std::vector<Value> props;  // 24 bytes
    Type* type;                // 8 bytes
    bool is_frozen;            // 1 byte
};

// ✅ GOOD: Hot data together
struct Object {
    // Hot fields (accessed frequently)
    Type* type;                // 8 bytes
    bool is_frozen;            // 1 byte
    uint8_t flags;             // 1 byte
    uint16_t property_count;   // 2 bytes
    
    // Cold fields (accessed rarely)
    std::string name;
    std::vector<Value> props;
};
```

### Branch Prediction

```cpp
// ❌ BAD: Unpredictable branches
if (value.isNumber()) {
    // ...
} else if (value.isString()) {
    // ...
} else if (value.isObject()) {
    // ...
}

// ✅ GOOD: Use jump table for common patterns
static void (*handlers[])(Value) = {
    handleNumber,
    handleString,
    handleObject,
    // ...
};

handlers[value.type()](value);
```

---

## 9. Security Guidelines

### Input Validation

```cpp
// Always validate user input
Value Array::get(int64_t index) {
    // Check bounds
    if (index < 0 || index >= static_cast<int64_t>(elements_.size())) {
        return Value::undefined();
    }
    
    return elements_[index];
}
```

### Integer Overflow

```cpp
// ❌ BAD: Possible overflow
int32_t size = a + b;

// ✅ GOOD: Check for overflow
if (a > INT32_MAX - b) {
    throw RangeError("Integer overflow");
}
int32_t size = a + b;

// ✅ BETTER: Use safe math
int32_t size = checked_add(a, b);  // Throws on overflow
```

### Memory Safety

```cpp
// ❌ BAD: Use after free
Object* obj = heap->allocate<Object>();
heap->collectGarbage();  // obj might be freed
obj->setProperty("x", value);  // Use after free!

// ✅ GOOD: Use GC handles
Handle<Object> obj = heap->allocate<Object>();
heap->collectGarbage();  // obj is protected
obj->setProperty("x", value);  // Safe
```

### Sanitization

```cpp
// Always escape user strings in error messages
void throwError(const std::string& user_input) {
    // ❌ BAD: Direct inclusion
    throw Error("Invalid input: " + user_input);
    
    // ✅ GOOD: Sanitize
    std::string sanitized = escapeString(user_input);
    throw Error("Invalid input: " + sanitized);
}
```

---

## 10. AI Assistant Instructions

### When Editing Code

1. **Always maintain architectural boundaries**
   - Core engine never depends on DevTools
   - Debug API is minimal and always present
   - Optional modules can be deleted

2. **Follow naming conventions**
   - Check existing code in the same module
   - Use consistent naming throughout

3. **Add tests for new features**
   - Unit tests for individual components
   - Integration tests for feature interactions
   - Update test262 runner if adding ES features

4. **Document public APIs**
   - Add JSDoc-style comments for public methods
   - Update docs/ if adding major features
   - Keep GEMINI.md updated

## Dont delete this file just keep it upadate  
 Use bash TODO.md for all relevent tasks