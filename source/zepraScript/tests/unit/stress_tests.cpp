/**
 * @file stress_tests.cpp
 * @brief Load Testing and Stress Testing for ZepraScript
 * 
 * Tests memory consumption, CPU load, and sandboxing limits.
 */

#include <gtest/gtest.h>
#include "runtime/execution/Sandbox.h"
#include "runtime/execution/IsolatedGlobal.h"
#include <vector>
#include <chrono>
#include <thread>
#include <atomic>
#include <iostream>
#include <iomanip>

using namespace Zepra::Runtime;

// =============================================================================
// Memory Load Tests
// =============================================================================

class MemoryLoadTests : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(MemoryLoadTests, AllocateUntilLimit) {
    ExecutionLimits limits;
    limits.maxHeapBytes = 10 * 1024 * 1024;  // 10MB limit
    ResourceMonitor monitor(limits);
    
    size_t allocations = 0;
    size_t chunkSize = 1024;  // 1KB chunks
    
    auto start = std::chrono::high_resolution_clock::now();
    
    while (monitor.checkHeapLimit()) {
        monitor.addHeapAllocation(chunkSize);
        allocations++;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "\n=== Memory Allocation Test ===" << std::endl;
    std::cout << "Total allocations: " << allocations << std::endl;
    std::cout << "Total heap used: " << (monitor.heapUsed() / 1024.0 / 1024.0) << " MB" << std::endl;
    std::cout << "Time taken: " << (duration.count() / 1000.0) << " ms" << std::endl;
    std::cout << "Allocations/ms: " << (allocations / (duration.count() / 1000.0)) << std::endl;
    
    EXPECT_GE(allocations, 10000);  // Should do at least 10K allocations
}

TEST_F(MemoryLoadTests, RapidAllocDealloc) {
    ExecutionLimits limits;
    limits.maxHeapBytes = 100 * 1024 * 1024;  // 100MB limit
    ResourceMonitor monitor(limits);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Simulate garbage collection cycles
    for (int cycle = 0; cycle < 100; ++cycle) {
        // Allocate 1MB
        for (int i = 0; i < 1000; ++i) {
            monitor.addHeapAllocation(1024);
        }
        
        // Deallocate 50%
        for (int i = 0; i < 500; ++i) {
            monitor.removeHeapAllocation(1024);
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "\n=== GC Simulation Test ===" << std::endl;
    std::cout << "Cycles: 100" << std::endl;
    std::cout << "Heap after cycles: " << (monitor.heapUsed() / 1024.0 / 1024.0) << " MB" << std::endl;
    std::cout << "Time taken: " << (duration.count() / 1000.0) << " ms" << std::endl;
    
    EXPECT_LT(duration.count(), 100000);  // Should complete in < 100ms
}

// =============================================================================
// CPU Load Tests
// =============================================================================

class CPULoadTests : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(CPULoadTests, InstructionThroughput) {
    ExecutionLimits limits;
    limits.maxInstructions = 100000000;  // 100M instructions
    ResourceMonitor monitor(limits);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Simulate 10M instruction batches
    for (int i = 0; i < 1000; ++i) {
        monitor.addInstructions(10000);  // 10K per batch
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    
    double instructionsPerSecond = (10000000.0 / duration.count()) * 1e9;
    
    std::cout << "\n=== Instruction Throughput Test ===" << std::endl;
    std::cout << "Instructions tracked: 10M" << std::endl;
    std::cout << "Time taken: " << (duration.count() / 1000.0) << " µs" << std::endl;
    std::cout << "Throughput: " << std::fixed << std::setprecision(0) 
              << (instructionsPerSecond / 1e9) << " B instructions/sec" << std::endl;
    
    EXPECT_GT(instructionsPerSecond, 1e9);  // Should track > 1B instr/sec
}

TEST_F(CPULoadTests, CallStackThroughput) {
    ExecutionLimits limits;
    limits.maxCallStackDepth = 1000000;
    ResourceMonitor monitor(limits);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Deep recursion simulation
    for (size_t i = 0; i < 500000; ++i) {
        monitor.pushCall();
    }
    for (size_t i = 0; i < 500000; ++i) {
        monitor.popCall();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    double opsPerSecond = (1000000.0 / duration.count()) * 1e6;
    
    std::cout << "\n=== Call Stack Throughput Test ===" << std::endl;
    std::cout << "Stack operations: 1M (500K push + 500K pop)" << std::endl;
    std::cout << "Time taken: " << (duration.count() / 1000.0) << " ms" << std::endl;
    std::cout << "Throughput: " << std::fixed << std::setprecision(0) 
              << (opsPerSecond / 1e6) << " M ops/sec" << std::endl;
    
    EXPECT_GT(opsPerSecond, 10e6);  // Should handle > 10M ops/sec
}

// =============================================================================
// Security/Sandboxing Tests
// =============================================================================

class SandboxLoadTests : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(SandboxLoadTests, CheckLimitsOverhead) {
    ExecutionLimits limits;
    limits.maxHeapBytes = 1024 * 1024 * 1024;
    limits.maxCallStackDepth = 100000;
    limits.maxInstructions = 1000000000;
    limits.maxExecutionTime = std::chrono::seconds(60);
    ResourceMonitor monitor(limits);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Simulate 1M limit checks (what happens in hot execution loop)
    for (int i = 0; i < 1000000; ++i) {
        bool ok = monitor.checkLimits();
        (void)ok;  // Prevent optimization
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    
    double checksPerSecond = (1000000.0 / duration.count()) * 1e9;
    double nsPerCheck = duration.count() / 1000000.0;
    
    std::cout << "\n=== Sandbox Check Overhead ===" << std::endl;
    std::cout << "Limit checks: 1M" << std::endl;
    std::cout << "Time taken: " << (duration.count() / 1e6) << " ms" << std::endl;
    std::cout << "Overhead per check: " << std::fixed << std::setprecision(1) 
              << nsPerCheck << " ns" << std::endl;
    std::cout << "Checks per second: " << std::fixed << std::setprecision(0) 
              << (checksPerSecond / 1e6) << " M/sec" << std::endl;
    
    // Sandbox overhead should be < 100ns per check
    EXPECT_LT(nsPerCheck, 100);
}

TEST_F(SandboxLoadTests, IsolatedGlobalLookupSpeed) {
    SecurityPolicy policy = SecurityPolicy::browser();
    IsolatedGlobal global(policy);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Simulate 1M global lookups
    std::vector<std::string> globals = {
        "Array", "Object", "String", "Number", "Boolean",
        "console", "Math", "JSON", "Promise", "Date"
    };
    
    for (int i = 0; i < 100000; ++i) {
        for (const auto& name : globals) {
            bool allowed = global.isAllowed(name);
            (void)allowed;
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    double lookupsPerSecond = (1000000.0 / duration.count()) * 1e6;
    
    std::cout << "\n=== Global Lookup Speed ===" << std::endl;
    std::cout << "Lookups: 1M" << std::endl;
    std::cout << "Time taken: " << (duration.count() / 1000.0) << " ms" << std::endl;
    std::cout << "Lookups/sec: " << std::fixed << std::setprecision(0) 
              << (lookupsPerSecond / 1e6) << " M/sec" << std::endl;
    
    EXPECT_GT(lookupsPerSecond, 1e6);  // Should do > 1M lookups/sec
}

TEST_F(SandboxLoadTests, SecurityPolicyEvaluation) {
    SecurityPolicy policy = SecurityPolicy::strict();
    IsolatedGlobal global(policy);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Test security checks
    for (int i = 0; i < 1000000; ++i) {
        bool e = global.isEvalAllowed();
        bool f = global.isFetchAllowed();
        bool w = global.isWorkersAllowed();
        bool wa = global.isWasmAllowed();
        (void)e; (void)f; (void)w; (void)wa;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "\n=== Security Policy Evaluation ===" << std::endl;
    std::cout << "Evaluations: 4M (1M x 4 checks)" << std::endl;
    std::cout << "Time taken: " << (duration.count() / 1000.0) << " ms" << std::endl;
    std::cout << "Checks/ms: " << std::fixed << std::setprecision(0) 
              << (4000000.0 / (duration.count() / 1000.0)) << std::endl;
    
    EXPECT_LT(duration.count(), 100000);  // < 100ms for 4M checks
}

// =============================================================================
// Concurrent Load Tests
// =============================================================================

class ConcurrencyTests : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(ConcurrencyTests, AtomicCounterScaling) {
    ExecutionLimits limits;
    ResourceMonitor monitor(limits);
    
    std::atomic<int> threadsCompleted{0};
    const int numThreads = 4;
    const int opsPerThread = 250000;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    std::vector<std::thread> threads;
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&monitor, &threadsCompleted, opsPerThread]() {
            for (int i = 0; i < opsPerThread; ++i) {
                monitor.addInstructions(1);
            }
            threadsCompleted++;
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "\n=== Concurrent Counter Test ===" << std::endl;
    std::cout << "Threads: " << numThreads << std::endl;
    std::cout << "Total ops: " << (numThreads * opsPerThread) << std::endl;
    std::cout << "Final count: " << monitor.instructionCount() << std::endl;
    std::cout << "Time taken: " << duration.count() << " ms" << std::endl;
    
    // Atomic counter should be accurate
    EXPECT_EQ(monitor.instructionCount(), numThreads * opsPerThread);
}

// =============================================================================
// Configuration Summary Test
// =============================================================================

TEST(ConfigurationTests, PrintSandboxConfig) {
    std::cout << "\n=== ZepraScript Sandbox Configuration ===" << std::endl;
    
    std::cout << "\n--- Default Limits ---" << std::endl;
    ExecutionLimits defaultLimits;
    std::cout << "Max Heap: " << (defaultLimits.maxHeapBytes / 1024.0 / 1024.0) << " MB" << std::endl;
    std::cout << "Max Stack: " << (defaultLimits.maxStackBytes / 1024.0 / 1024.0) << " MB" << std::endl;
    std::cout << "Max Call Depth: " << defaultLimits.maxCallStackDepth << std::endl;
    std::cout << "Max String Length: " << (defaultLimits.maxStringLength / 1024.0 / 1024.0) << " MB" << std::endl;
    std::cout << "Max Array Length: " << defaultLimits.maxArrayLength << std::endl;
    
    std::cout << "\n--- Browser Limits ---" << std::endl;
    auto browserLimits = ExecutionLimits::browser();
    std::cout << "Max Heap: " << (browserLimits.maxHeapBytes / 1024.0 / 1024.0) << " MB" << std::endl;
    std::cout << "Max Execution Time: " << browserLimits.maxExecutionTime.count() << " ms" << std::endl;
    std::cout << "Max Instructions: " << browserLimits.maxInstructions << std::endl;
    
    std::cout << "\n--- Untrusted Limits ---" << std::endl;
    auto untrustedLimits = ExecutionLimits::untrusted();
    std::cout << "Max Heap: " << (untrustedLimits.maxHeapBytes / 1024.0 / 1024.0) << " MB" << std::endl;
    std::cout << "Max Execution Time: " << untrustedLimits.maxExecutionTime.count() << " ms" << std::endl;
    std::cout << "Max Loop Iterations: " << untrustedLimits.maxLoopIterations << std::endl;
    
    std::cout << "\n--- Security Policy (Browser) ---" << std::endl;
    auto browserPolicy = SecurityPolicy::browser();
    std::cout << "Allow Eval: " << (browserPolicy.allowEval ? "Yes" : "No") << std::endl;
    std::cout << "Allow Fetch: " << (browserPolicy.allowFetch ? "Yes" : "No") << std::endl;
    std::cout << "Allow Workers: " << (browserPolicy.allowWorkers ? "Yes" : "No") << std::endl;
    std::cout << "Allow WASM: " << (browserPolicy.allowWasm ? "Yes" : "No") << std::endl;
    
    std::cout << "\n--- Security Policy (Strict) ---" << std::endl;
    auto strictPolicy = SecurityPolicy::strict();
    std::cout << "Allow Eval: " << (strictPolicy.allowEval ? "Yes" : "No") << std::endl;
    std::cout << "Allow Fetch: " << (strictPolicy.allowFetch ? "Yes" : "No") << std::endl;
    std::cout << "Allow Workers: " << (strictPolicy.allowWorkers ? "Yes" : "No") << std::endl;
    std::cout << "Allow WASM: " << (strictPolicy.allowWasm ? "Yes" : "No") << std::endl;
    std::cout << "Allow Network: " << (strictPolicy.allowNetworkAccess ? "Yes" : "No") << std::endl;
    
    EXPECT_TRUE(true);  // Always passes, just prints config
}
