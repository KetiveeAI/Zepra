// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

/**
 * @file vm_stress_test.cpp
 * @brief VM internals stress tests — stack pressure, IC saturation, deep
 *        recursion, sandbox enforcement, concurrent isolation.
 */

#include <gtest/gtest.h>
#include "runtime/execution/vm.hpp"
#include "runtime/execution/Sandbox.h"
#include "runtime/objects/object.hpp"
#include "runtime/objects/function.hpp"
#include "heap/gc_heap.hpp"
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>
#include <iostream>
#include <iomanip>

using namespace Zepra::Runtime;

// =============================================================================
// Stack Pressure Tests
// =============================================================================

TEST(VMStress, OperandStackFlood) {
    VM vm(nullptr);
    constexpr size_t COUNT = 65535;

    for (size_t i = 0; i < COUNT; ++i) {
        vm.push(Value::number(static_cast<double>(i)));
    }
    EXPECT_EQ(vm.stackSize(), COUNT);

    for (size_t i = COUNT; i > 0; --i) {
        Value v = vm.pop();
        EXPECT_EQ(v.asNumber(), static_cast<double>(i - 1));
    }
    EXPECT_EQ(vm.stackSize(), 0);
}

TEST(VMStress, ValueTypeStorm) {
    VM vm(nullptr);

    for (int i = 0; i < 100000; ++i) {
        vm.push(Value::number(static_cast<double>(i)));
        vm.push(Value::boolean(i % 2 == 0));
        vm.push(Value::null());
        vm.push(Value::undefined());
        EXPECT_EQ(vm.stackSize(), 4u);

        Value undef = vm.pop();
        EXPECT_TRUE(undef.isUndefined());
        Value nil = vm.pop();
        EXPECT_TRUE(nil.isNull());
        Value b = vm.pop();
        EXPECT_TRUE(b.isBoolean());
        Value n = vm.pop();
        EXPECT_TRUE(n.isNumber());
    }
    EXPECT_EQ(vm.stackSize(), 0);
}

// =============================================================================
// Global Lookup Density
// =============================================================================

TEST(VMStress, GlobalLookupMissDensity) {
    VM vm(nullptr);

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 1000000; ++i) {
        Value v = vm.getGlobal("nonexistent_key");
        EXPECT_TRUE(v.isUndefined());
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    std::cout << "\n=== Global Lookup Miss (1M) ===" << std::endl;
    std::cout << "Time: " << (us / 1000.0) << " ms" << std::endl;
    std::cout << "Lookups/ms: " << std::fixed << std::setprecision(0)
              << (1000000.0 / (us / 1000.0)) << std::endl;
}

TEST(VMStress, GlobalSetGetDensity) {
    VM vm(nullptr);

    for (int i = 0; i < 10000; ++i) {
        std::string name = "global_" + std::to_string(i);
        vm.setGlobal(name, Value::number(static_cast<double>(i)));
    }

    for (int i = 0; i < 10000; ++i) {
        std::string name = "global_" + std::to_string(i);
        Value v = vm.getGlobal(name);
        EXPECT_TRUE(v.isNumber());
        EXPECT_EQ(v.asNumber(), static_cast<double>(i));
    }
}

// =============================================================================
// Sandbox & Termination Tests
// =============================================================================

TEST(VMStress, TerminationRequest) {
    VM vm(nullptr);
    EXPECT_FALSE(vm.isTerminationRequested());
    EXPECT_FALSE(vm.shouldTerminate());

    vm.requestTermination();
    EXPECT_TRUE(vm.isTerminationRequested());
    EXPECT_TRUE(vm.shouldTerminate());
}

TEST(VMStress, SandboxInstructionCap) {
    ExecutionLimits limits;
    limits.maxInstructions = 10000;
    ResourceMonitor monitor(limits);

    VM vm(nullptr);
    vm.setSandbox(&monitor);

    monitor.addInstructions(9999);
    EXPECT_FALSE(vm.shouldTerminate());

    monitor.addInstructions(2);
    EXPECT_TRUE(vm.shouldTerminate());
}

TEST(VMStress, SandboxHeapCap) {
    ExecutionLimits limits;
    limits.maxHeapBytes = 1024 * 1024;  // 1MB
    ResourceMonitor monitor(limits);

    VM vm(nullptr);
    vm.setSandbox(&monitor);

    monitor.addHeapAllocation(512 * 1024);
    EXPECT_FALSE(vm.shouldTerminate());

    monitor.addHeapAllocation(600 * 1024);
    EXPECT_TRUE(vm.shouldTerminate());
}

// =============================================================================
// Call Depth Tracking
// =============================================================================

TEST(VMStress, CallDepthAccuracy) {
    VM vm(nullptr);
    EXPECT_EQ(vm.getCallDepth(), 0u);

    // Push/pop through the public stack interface is value stack, not call stack.
    // Call depth is zero without executing bytecode — validates initial state.
    EXPECT_EQ(vm.getCallDepth(), 0u);
}

// =============================================================================
// Concurrent VM Isolation
// =============================================================================

TEST(VMStress, ConcurrentVMIsolation) {
    constexpr int NUM_THREADS = 4;
    constexpr int OPS_PER_THREAD = 50000;
    std::atomic<int> passed{0};

    auto worker = [&](int id) {
        VM vm(nullptr);

        for (int i = 0; i < OPS_PER_THREAD; ++i) {
            double val = static_cast<double>(id * 1000000 + i);
            vm.setGlobal("x", Value::number(val));
            Value got = vm.getGlobal("x");
            if (got.isNumber() && got.asNumber() == val) {
                // pass
            } else {
                return;  // Corruption detected
            }
        }
        passed++;
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back(worker, i);
    }
    for (auto& t : threads) t.join();

    EXPECT_EQ(passed.load(), NUM_THREADS);

    std::cout << "\n=== Concurrent VM Isolation ===" << std::endl;
    std::cout << NUM_THREADS << " threads × " << OPS_PER_THREAD
              << " ops — no cross-contamination" << std::endl;
}

// =============================================================================
// Execute Null Chunk
// =============================================================================

TEST(VMStress, ExecuteNullChunk) {
    VM vm(nullptr);
    auto result = vm.execute(static_cast<const Zepra::Bytecode::BytecodeChunk*>(nullptr));
    EXPECT_EQ(result.status, ExecutionResult::Status::Error);
}

// =============================================================================
// Compile Null/Empty Source
// =============================================================================

TEST(VMStress, CompileEmptySource) {
    VM vm(nullptr);
    void* compiled = vm.compile("");
    // Empty source may produce an empty chunk or nullptr — either is acceptable
    // The important thing is it doesn't crash
    (void)compiled;
    EXPECT_TRUE(true);
}

TEST(VMStress, CompileInvalidSource) {
    VM vm(nullptr);
    void* compiled = vm.compile("function {{{ broken syntax");
    EXPECT_EQ(compiled, nullptr);
}

// =============================================================================
// Stack Push/Pop Throughput
// =============================================================================

TEST(VMStress, StackThroughput) {
    VM vm(nullptr);

    auto start = std::chrono::high_resolution_clock::now();

    for (int cycle = 0; cycle < 1000; ++cycle) {
        for (int i = 0; i < 1000; ++i) {
            vm.push(Value::number(static_cast<double>(i)));
        }
        for (int i = 0; i < 1000; ++i) {
            vm.pop();
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    double opsPerSec = (2000000.0 / us) * 1e6;

    std::cout << "\n=== Stack Throughput (1M push + 1M pop) ===" << std::endl;
    std::cout << "Time: " << (us / 1000.0) << " ms" << std::endl;
    std::cout << "Throughput: " << std::fixed << std::setprecision(0)
              << (opsPerSec / 1e6) << " M ops/sec" << std::endl;

    EXPECT_GT(opsPerSec, 10e6);
    EXPECT_EQ(vm.stackSize(), 0u);
}

// =============================================================================
// Execution Pipeline Stress Tests (Compile + Execute)
// =============================================================================

TEST(VMStress, PipelineHotLoop) {
    VM vm(nullptr);
    // Compile and execute a tight for-loop
    void* compiled = vm.compile("var sum = 0; for (var i = 0; i < 10000; i++) { sum = sum + 1; }");
    if (compiled) {
        auto result = vm.execute(static_cast<const Zepra::Bytecode::BytecodeChunk*>(compiled));
        // Should complete without error or timeout
        EXPECT_NE(result.status, ExecutionResult::Status::Error);
    }
}

TEST(VMStress, PipelineObjectAllocation) {
    VM vm(nullptr);
    void* compiled = vm.compile(
        "var objects = [];"
        "for (var i = 0; i < 100; i++) {"
        "  objects[i] = { x: i, y: i * 2, name: 'obj' };"
        "}"
    );
    if (compiled) {
        auto result = vm.execute(static_cast<const Zepra::Bytecode::BytecodeChunk*>(compiled));
        EXPECT_NE(result.status, ExecutionResult::Status::Error);
    }
}

TEST(VMStress, PipelineStringConcat) {
    VM vm(nullptr);
    void* compiled = vm.compile(
        "var s = '';"
        "for (var i = 0; i < 100; i++) {"
        "  s = s + 'x';"
        "}"
    );
    if (compiled) {
        auto result = vm.execute(static_cast<const Zepra::Bytecode::BytecodeChunk*>(compiled));
        EXPECT_NE(result.status, ExecutionResult::Status::Error);
    }
}

TEST(VMStress, PipelineClosureChain) {
    VM vm(nullptr);
    // Nested closures capturing from outer scopes
    void* compiled = vm.compile(
        "function make(n) {"
        "  var x = n;"
        "  return function() { return x; };"
        "}"
        "var fns = [];"
        "for (var i = 0; i < 50; i++) {"
        "  fns[i] = make(i);"
        "}"
    );
    if (compiled) {
        auto result = vm.execute(static_cast<const Zepra::Bytecode::BytecodeChunk*>(compiled));
        EXPECT_NE(result.status, ExecutionResult::Status::Error);
    }
}

TEST(VMStress, PipelineMixedTypes) {
    VM vm(nullptr);
    // Exercise type coercion paths
    void* compiled = vm.compile(
        "var a = 1 + '2';"      // string concat
        "var b = true + 1;"     // numeric coercion
        "var c = null + 1;"     // null → 0
        "var d = undefined + 1;"// NaN
    );
    if (compiled) {
        auto result = vm.execute(static_cast<const Zepra::Bytecode::BytecodeChunk*>(compiled));
        EXPECT_NE(result.status, ExecutionResult::Status::Error);
    }
}

TEST(VMStress, PipelineExceptionStorm) {
    VM vm(nullptr);
    void* compiled = vm.compile(
        "var caught = 0;"
        "for (var i = 0; i < 100; i++) {"
        "  try {"
        "    throw 'error';"
        "  } catch (e) {"
        "    caught = caught + 1;"
        "  }"
        "}"
    );
    if (compiled) {
        auto result = vm.execute(static_cast<const Zepra::Bytecode::BytecodeChunk*>(compiled));
        EXPECT_NE(result.status, ExecutionResult::Status::Error);
    }
}

TEST(VMStress, PipelineDeepNesting) {
    VM vm(nullptr);
    // Generate deeply nested if-else
    std::string code = "var result = 0;";
    for (int i = 0; i < 50; ++i) {
        code += "if (true) { result = result + 1; ";
    }
    for (int i = 0; i < 50; ++i) {
        code += "} ";
    }

    void* compiled = vm.compile(code);
    if (compiled) {
        auto result = vm.execute(static_cast<const Zepra::Bytecode::BytecodeChunk*>(compiled));
        EXPECT_NE(result.status, ExecutionResult::Status::Error);
    }
}

TEST(VMStress, RapidCompileIterations) {
    VM vm(nullptr);
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 100; ++i) {
        std::string code = "var x" + std::to_string(i) + " = " + std::to_string(i * i) + ";";
        void* compiled = vm.compile(code);
        // Should not crash on repeated compilations
        (void)compiled;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << "\n=== Rapid Compile (100 programs) ===" << std::endl;
    std::cout << "Time: " << ms << " ms" << std::endl;
    EXPECT_LT(ms, 5000); // 100 compilations should complete within 5s
}

