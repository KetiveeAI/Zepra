/**
 * @file sandbox_tests.cpp
 * @brief Unit tests for Sandbox and Security Infrastructure
 */

#include <gtest/gtest.h>
#include "runtime/execution/Sandbox.h"
#include "runtime/execution/IsolatedGlobal.h"
#include <chrono>
#include <thread>

using namespace Zepra::Runtime;

// =============================================================================
// ExecutionLimits Tests
// =============================================================================

class ExecutionLimitsTests : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(ExecutionLimitsTests, DefaultLimits) {
    ExecutionLimits limits;
    
    EXPECT_EQ(limits.maxHeapBytes, 256 * 1024 * 1024);
    EXPECT_EQ(limits.maxStackBytes, 8 * 1024 * 1024);
    EXPECT_EQ(limits.maxCallStackDepth, 10000);
}

TEST_F(ExecutionLimitsTests, BrowserLimits) {
    auto limits = ExecutionLimits::browser();
    
    EXPECT_EQ(limits.maxHeapBytes, 512 * 1024 * 1024);
    EXPECT_EQ(limits.maxExecutionTime, std::chrono::seconds(30));
}

TEST_F(ExecutionLimitsTests, UntrustedLimits) {
    auto limits = ExecutionLimits::untrusted();
    
    EXPECT_EQ(limits.maxHeapBytes, 64 * 1024 * 1024);
    EXPECT_EQ(limits.maxExecutionTime, std::chrono::seconds(5));
    EXPECT_EQ(limits.maxCallStackDepth, 1000);
}

// =============================================================================
// SecurityPolicy Tests
// =============================================================================

class SecurityPolicyTests : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(SecurityPolicyTests, BrowserPolicy) {
    auto policy = SecurityPolicy::browser();
    
    EXPECT_FALSE(policy.allowEval);
    EXPECT_TRUE(policy.allowFetch);
    EXPECT_TRUE(policy.allowWorkers);
    EXPECT_TRUE(policy.allowWasm);
}

TEST_F(SecurityPolicyTests, StrictPolicy) {
    auto policy = SecurityPolicy::strict();
    
    EXPECT_FALSE(policy.allowEval);
    EXPECT_FALSE(policy.allowFetch);
    EXPECT_FALSE(policy.allowWorkers);
    EXPECT_FALSE(policy.allowWasm);
    EXPECT_FALSE(policy.allowNetworkAccess);
}

// =============================================================================
// ResourceMonitor Tests
// =============================================================================

class ResourceMonitorTests : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(ResourceMonitorTests, InstructionCounting) {
    auto limits = ExecutionLimits::browser();
    ResourceMonitor monitor(limits);
    
    EXPECT_EQ(monitor.instructionCount(), 0);
    
    monitor.addInstructions(1000);
    EXPECT_EQ(monitor.instructionCount(), 1000);
    
    monitor.addInstructions(500);
    EXPECT_EQ(monitor.instructionCount(), 1500);
}

TEST_F(ResourceMonitorTests, InstructionLimit) {
    ExecutionLimits limits;
    limits.maxInstructions = 1000;
    ResourceMonitor monitor(limits);
    
    EXPECT_TRUE(monitor.checkInstructionLimit());
    
    monitor.addInstructions(500);
    EXPECT_TRUE(monitor.checkInstructionLimit());
    
    monitor.addInstructions(600);  // Now at 1100
    EXPECT_FALSE(monitor.checkInstructionLimit());
}

TEST_F(ResourceMonitorTests, HeapTracking) {
    auto limits = ExecutionLimits::browser();
    ResourceMonitor monitor(limits);
    
    EXPECT_EQ(monitor.heapUsed(), 0);
    
    monitor.addHeapAllocation(1024);
    EXPECT_EQ(monitor.heapUsed(), 1024);
    
    monitor.removeHeapAllocation(512);
    EXPECT_EQ(monitor.heapUsed(), 512);
}

TEST_F(ResourceMonitorTests, HeapLimit) {
    ExecutionLimits limits;
    limits.maxHeapBytes = 1024;
    ResourceMonitor monitor(limits);
    
    EXPECT_TRUE(monitor.checkHeapLimit());
    
    monitor.addHeapAllocation(512);
    EXPECT_TRUE(monitor.checkHeapLimit());
    
    monitor.addHeapAllocation(600);  // Now at 1112
    EXPECT_FALSE(monitor.checkHeapLimit());
}

TEST_F(ResourceMonitorTests, CallStackTracking) {
    auto limits = ExecutionLimits::browser();
    ResourceMonitor monitor(limits);
    
    EXPECT_EQ(monitor.callDepth(), 0);
    
    monitor.pushCall();
    monitor.pushCall();
    EXPECT_EQ(monitor.callDepth(), 2);
    
    monitor.popCall();
    EXPECT_EQ(monitor.callDepth(), 1);
}

TEST_F(ResourceMonitorTests, StackLimit) {
    ExecutionLimits limits;
    limits.maxCallStackDepth = 3;
    ResourceMonitor monitor(limits);
    
    EXPECT_TRUE(monitor.checkStackLimit());
    
    monitor.pushCall();
    monitor.pushCall();
    EXPECT_TRUE(monitor.checkStackLimit());
    
    monitor.pushCall();
    monitor.pushCall();  // Now at 4
    EXPECT_FALSE(monitor.checkStackLimit());
}

TEST_F(ResourceMonitorTests, TerminationRequest) {
    auto limits = ExecutionLimits::browser();
    ResourceMonitor monitor(limits);
    
    EXPECT_FALSE(monitor.isTerminationRequested());
    
    monitor.requestTermination();
    EXPECT_TRUE(monitor.isTerminationRequested());
}

// =============================================================================
// IsolatedGlobal Tests
// =============================================================================

class IsolatedGlobalTests : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(IsolatedGlobalTests, SafeGlobalsAllowed) {
    auto policy = SecurityPolicy::browser();
    IsolatedGlobal global(policy);
    
    EXPECT_TRUE(global.isAllowed("Object"));
    EXPECT_TRUE(global.isAllowed("Array"));
    EXPECT_TRUE(global.isAllowed("String"));
    EXPECT_TRUE(global.isAllowed("Math"));
    EXPECT_TRUE(global.isAllowed("JSON"));
}

TEST_F(IsolatedGlobalTests, DangerousGlobalsBlocked) {
    auto policy = SecurityPolicy::browser();
    IsolatedGlobal global(policy);
    
    EXPECT_FALSE(global.isAllowed("eval"));
    EXPECT_FALSE(global.isAllowed("Function"));
    EXPECT_FALSE(global.isAllowed("__proto__"));
}

TEST_F(IsolatedGlobalTests, ExplicitDenyList) {
    SecurityPolicy policy;
    policy.deniedGlobals.insert("console");
    IsolatedGlobal global(policy);
    
    EXPECT_FALSE(global.isAllowed("console"));
}

TEST_F(IsolatedGlobalTests, EvalControl) {
    {
        SecurityPolicy policy;
        policy.allowEval = false;
        IsolatedGlobal global(policy);
        EXPECT_FALSE(global.isEvalAllowed());
    }
    {
        SecurityPolicy policy;
        policy.allowEval = true;
        IsolatedGlobal global(policy);
        EXPECT_TRUE(global.isEvalAllowed());
    }
}

TEST_F(IsolatedGlobalTests, FetchControl) {
    {
        SecurityPolicy policy;
        policy.allowFetch = false;
        IsolatedGlobal global(policy);
        EXPECT_FALSE(global.isFetchAllowed());
    }
    {
        SecurityPolicy policy;
        policy.allowFetch = true;
        IsolatedGlobal global(policy);
        EXPECT_TRUE(global.isFetchAllowed());
    }
}

// =============================================================================
// SecureContext Tests
// =============================================================================

class SecureContextTests : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(SecureContextTests, CreationWithBrowserConfig) {
    auto config = SandboxConfig::browser();
    SecureContext ctx(config);
    
    EXPECT_TRUE(ctx.global().isFetchAllowed());
    EXPECT_TRUE(ctx.global().isWorkersAllowed());
}

TEST_F(SecureContextTests, CreationWithStrictConfig) {
    auto config = SandboxConfig::strict();
    SecureContext ctx(config);
    
    EXPECT_FALSE(ctx.global().isFetchAllowed());
    EXPECT_FALSE(ctx.global().isWorkersAllowed());
}

// =============================================================================
// SecurityError Tests  
// =============================================================================

TEST(SecurityErrorTests, TimeoutError) {
    SecurityError error(SecurityError::Type::Timeout, "Execution timed out");
    
    EXPECT_EQ(error.type(), SecurityError::Type::Timeout);
    EXPECT_STREQ(error.what(), "Execution timed out");
}

TEST(SecurityErrorTests, MemoryLimitError) {
    SecurityError error(SecurityError::Type::MemoryLimit, "Heap limit exceeded");
    
    EXPECT_EQ(error.type(), SecurityError::Type::MemoryLimit);
}
