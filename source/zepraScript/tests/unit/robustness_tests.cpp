/**
 * @file robustness_tests.cpp
 * @brief Performance and Robustness Tests
 * 
 * Tests for stack overflow protection, GC stress, and memory safety.
 */

#include <gtest/gtest.h>
#include "runtime/execution/Sandbox.h"
#include "safety/CrashBoundary.h"
#include <vector>
#include <thread>
#include <chrono>

using namespace Zepra::Runtime;
using namespace Zepra::Safety;

// =============================================================================
// Stack Overflow Protection Tests
// =============================================================================

class StackOverflowTests : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(StackOverflowTests, ResourceMonitorStackDepthLimit) {
    ExecutionLimits limits;
    limits.maxCallStackDepth = 100;
    ResourceMonitor monitor(limits);
    
    // Simulate deep recursion
    for (size_t i = 0; i < 99; ++i) {
        monitor.pushCall();
        EXPECT_TRUE(monitor.checkStackLimit());
    }
    
    // At depth 99, should still be OK
    EXPECT_TRUE(monitor.checkStackLimit());
    
    // Push to 100 - now at limit (100 < 100 is FALSE)
    monitor.pushCall();
    EXPECT_FALSE(monitor.checkStackLimit());
}

TEST_F(StackOverflowTests, PopCallReducesDepth) {
    ExecutionLimits limits;
    limits.maxCallStackDepth = 50;
    ResourceMonitor monitor(limits);
    
    for (size_t i = 0; i < 60; ++i) {
        monitor.pushCall();
    }
    EXPECT_FALSE(monitor.checkStackLimit());
    
    // Pop back under limit
    for (size_t i = 0; i < 20; ++i) {
        monitor.popCall();
    }
    EXPECT_TRUE(monitor.checkStackLimit());
}

TEST_F(StackOverflowTests, SecurityErrorOnOverflow) {
    auto error = SecurityError(SecurityError::Type::StackOverflow, "Stack depth exceeded");
    EXPECT_EQ(error.type(), SecurityError::Type::StackOverflow);
    EXPECT_STREQ(error.what(), "Stack depth exceeded");
}

// =============================================================================
// GC Stress Tests
// =============================================================================

class GCStressTests : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(GCStressTests, HeapAllocationTracking) {
    ExecutionLimits limits;
    limits.maxHeapBytes = 1024 * 1024;  // 1MB
    ResourceMonitor monitor(limits);
    
    // Simulate many small allocations
    for (size_t i = 0; i < 1000; ++i) {
        monitor.addHeapAllocation(512);  // 512 bytes each
    }
    
    EXPECT_EQ(monitor.heapUsed(), 512 * 1000);
    EXPECT_TRUE(monitor.checkHeapLimit());
}

TEST_F(GCStressTests, HeapLimitExceeded) {
    ExecutionLimits limits;
    limits.maxHeapBytes = 1000;
    ResourceMonitor monitor(limits);
    
    monitor.addHeapAllocation(500);
    EXPECT_TRUE(monitor.checkHeapLimit());
    
    monitor.addHeapAllocation(600);
    EXPECT_FALSE(monitor.checkHeapLimit());
}

TEST_F(GCStressTests, HeapDeallocation) {
    ExecutionLimits limits;
    limits.maxHeapBytes = 2000;
    ResourceMonitor monitor(limits);
    
    monitor.addHeapAllocation(1500);
    EXPECT_TRUE(monitor.checkHeapLimit());
    
    monitor.addHeapAllocation(1000);
    EXPECT_FALSE(monitor.checkHeapLimit());
    
    // Deallocate to get back under limit
    monitor.removeHeapAllocation(1000);
    EXPECT_TRUE(monitor.checkHeapLimit());
}

TEST_F(GCStressTests, RapidAllocationDeallocation) {
    ExecutionLimits limits;
    limits.maxHeapBytes = 10000;
    ResourceMonitor monitor(limits);
    
    // Simulate rapid alloc/dealloc cycles
    for (size_t i = 0; i < 1000; ++i) {
        monitor.addHeapAllocation(100);
        if (i % 10 == 0) {
            monitor.removeHeapAllocation(500);
        }
    }
    
    // Should have allocated 100*1000 = 100K, deallocated 500*100 = 50K
    // Net: 50K, but capped at 0 minimum for deallocs
}

// =============================================================================
// Memory Safety Tests
// =============================================================================

class MemorySafetyTests : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(MemorySafetyTests, LimitsDefaultValues) {
    ExecutionLimits limits;
    
    EXPECT_GT(limits.maxHeapBytes, 0);
    EXPECT_GT(limits.maxStackBytes, 0);
    EXPECT_GT(limits.maxCallStackDepth, 0);
    EXPECT_GT(limits.maxStringLength, 0);
}

TEST_F(MemorySafetyTests, UntrustedLimitsAreStrict) {
    auto untrusted = ExecutionLimits::untrusted();
    auto browser = ExecutionLimits::browser();
    
    EXPECT_LT(untrusted.maxHeapBytes, browser.maxHeapBytes);
    EXPECT_LT(untrusted.maxCallStackDepth, browser.maxCallStackDepth);
}

TEST_F(MemorySafetyTests, ArrayLengthLimit) {
    ExecutionLimits limits;
    
    // JavaScript max array length is 2^32 - 1
    EXPECT_EQ(limits.maxArrayLength, 4294967295u);
}

// =============================================================================
// Execution Timeout Tests
// =============================================================================

class TimeoutTests : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(TimeoutTests, NoTimeoutWhenUnlimited) {
    ExecutionLimits limits;
    limits.maxExecutionTime = std::chrono::milliseconds(0);
    ResourceMonitor monitor(limits);
    
    // Should always pass when unlimited
    EXPECT_TRUE(monitor.checkTimeout());
}

TEST_F(TimeoutTests, TimeoutCheckWithinLimit) {
    ExecutionLimits limits;
    limits.maxExecutionTime = std::chrono::seconds(5);
    ResourceMonitor monitor(limits);
    
    // Immediately after creation, should not be timed out
    EXPECT_TRUE(monitor.checkTimeout());
}

TEST_F(TimeoutTests, CheckLimitsCombined) {
    ExecutionLimits limits;
    limits.maxHeapBytes = 1024;
    limits.maxCallStackDepth = 100;
    limits.maxInstructions = 10000;
    ResourceMonitor monitor(limits);
    
    // All within limits
    monitor.addHeapAllocation(512);
    monitor.pushCall();
    monitor.addInstructions(5000);
    EXPECT_TRUE(monitor.checkLimits());
    
    // Exceed one limit (heap)
    monitor.addHeapAllocation(1000);
    EXPECT_FALSE(monitor.checkLimits());
}

// =============================================================================
// Instruction Limit Tests
// =============================================================================

class InstructionLimitTests : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(InstructionLimitTests, UnlimitedInstructions) {
    ExecutionLimits limits;
    limits.maxInstructions = 0;  // Unlimited
    ResourceMonitor monitor(limits);
    
    monitor.addInstructions(1000000000);
    EXPECT_TRUE(monitor.checkInstructionLimit());
}

TEST_F(InstructionLimitTests, LimitedInstructions) {
    ExecutionLimits limits;
    limits.maxInstructions = 10000;
    ResourceMonitor monitor(limits);
    
    monitor.addInstructions(5000);
    EXPECT_TRUE(monitor.checkInstructionLimit());
    
    monitor.addInstructions(6000);  // Total 11000
    EXPECT_FALSE(monitor.checkInstructionLimit());
}

TEST_F(InstructionLimitTests, InstructionCounting) {
    ExecutionLimits limits;
    ResourceMonitor monitor(limits);
    
    EXPECT_EQ(monitor.instructionCount(), 0);
    
    monitor.addInstructions(100);
    EXPECT_EQ(monitor.instructionCount(), 100);
    
    monitor.addInstructions(50);
    EXPECT_EQ(monitor.instructionCount(), 150);
}

// =============================================================================
// Termination Request Tests
// =============================================================================

class TerminationTests : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(TerminationTests, RequestTermination) {
    ExecutionLimits limits;
    ResourceMonitor monitor(limits);
    
    EXPECT_FALSE(monitor.isTerminationRequested());
    
    monitor.requestTermination();
    EXPECT_TRUE(monitor.isTerminationRequested());
}

TEST_F(TerminationTests, TerminationIsPersistent) {
    ExecutionLimits limits;
    ResourceMonitor monitor(limits);
    
    monitor.requestTermination();
    
    // Should stay true
    EXPECT_TRUE(monitor.isTerminationRequested());
    EXPECT_TRUE(monitor.isTerminationRequested());
    EXPECT_TRUE(monitor.isTerminationRequested());
}

// =============================================================================
// Elapsed Time Tracking
// =============================================================================

TEST(ElapsedTimeTests, ElapsedTimeTracking) {
    ExecutionLimits limits;
    ResourceMonitor monitor(limits);
    
    auto elapsed = monitor.elapsedTime();
    EXPECT_GE(elapsed.count(), 0);  // Should be non-negative
}

// =============================================================================
// Benchmark Validation Tests
// =============================================================================

TEST(BenchmarkTests, ResourceMonitorOverhead) {
    ExecutionLimits limits;
    ResourceMonitor monitor(limits);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Simulate 1M instruction checks
    for (size_t i = 0; i < 1000000; ++i) {
        monitor.addInstructions(1);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Should complete in reasonable time (< 100ms for 1M ops)
    EXPECT_LT(duration.count(), 100000);
}

TEST(BenchmarkTests, StackTrackingOverhead) {
    ExecutionLimits limits;
    limits.maxCallStackDepth = 1000000;
    ResourceMonitor monitor(limits);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < 100000; ++i) {
        monitor.pushCall();
    }
    for (size_t i = 0; i < 100000; ++i) {
        monitor.popCall();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Should be fast (< 50ms for 200K ops)
    EXPECT_LT(duration.count(), 50000);
}
