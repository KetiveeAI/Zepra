/**
 * @file Test262Harness.h
 * @brief Test262 ECMAScript conformance test harness
 * 
 * Implements:
 * - Test262 test loading and parsing
 * - Harness file injection ($262, assert, etc.)
 * - Async test support
 * - Test result reporting
 */

#pragma once

#include "runtime/objects/value.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <filesystem>

namespace Zepra::Spec {

// =============================================================================
// Test Metadata
// =============================================================================

struct TestMetadata {
    std::string description;
    std::string esid;                   // ES spec section ID
    std::vector<std::string> features;  // Required features
    std::vector<std::string> includes;  // Harness files to include
    std::vector<std::string> flags;     // async, module, raw, etc.
    std::string negative;               // Expected error type
    std::string negativePhase;          // parse, resolution, runtime
    std::string locale;                 // For Intl tests
    
    bool isAsync() const {
        return std::find(flags.begin(), flags.end(), "async") != flags.end();
    }
    
    bool isModule() const {
        return std::find(flags.begin(), flags.end(), "module") != flags.end();
    }
    
    bool isStrict() const {
        return std::find(flags.begin(), flags.end(), "onlyStrict") != flags.end();
    }
    
    bool isNoStrict() const {
        return std::find(flags.begin(), flags.end(), "noStrict") != flags.end();
    }
    
    bool isNegative() const {
        return !negative.empty();
    }
};

// =============================================================================
// Test Result
// =============================================================================

enum class TestResult : uint8_t {
    Pass,
    Fail,
    Skip,
    Timeout,
    Error
};

struct TestOutcome {
    TestResult result;
    std::string testPath;
    std::string message;
    double duration;  // milliseconds
    
    std::string resultString() const {
        switch (result) {
            case TestResult::Pass: return "PASS";
            case TestResult::Fail: return "FAIL";
            case TestResult::Skip: return "SKIP";
            case TestResult::Timeout: return "TIMEOUT";
            case TestResult::Error: return "ERROR";
        }
        return "UNKNOWN";
    }
};

// =============================================================================
// Harness Globals ($262, assert, etc.)
// =============================================================================

class HarnessGlobals {
public:
    HarnessGlobals() = default;
    
    // $262 agent helpers
    Runtime::Value agent_start(const std::string& script);
    Runtime::Value agent_broadcast(Runtime::Value buffer);
    Runtime::Value agent_getReport();
    Runtime::Value agent_sleep(double ms);
    Runtime::Value agent_monotonicNow();
    
    // $262 object
    Runtime::Value createRealm();
    Runtime::Value detachArrayBuffer(Runtime::Value buffer);
    Runtime::Value evalScript(const std::string& code);
    Runtime::Value gc();
    
    // assert helpers
    static void assert_(bool condition, const std::string& message = "");
    static void assertSameValue(Runtime::Value actual, Runtime::Value expected);
    static void assertNotSameValue(Runtime::Value actual, Runtime::Value expected);
    static void assertThrows(const std::string& errorType, std::function<void()> fn);
    
    // Install into global object
    void install(Runtime::Value globalObject);
};

// =============================================================================
// Test262 Test Runner
// =============================================================================

class Test262Harness {
public:
    using TestCallback = std::function<void(const TestOutcome&)>;
    using ExecuteCallback = std::function<Runtime::Value(const std::string& code, bool isModule)>;
    
    Test262Harness(const std::filesystem::path& test262Path);
    
    // =========================================================================
    // Configuration
    // =========================================================================
    
    void setExecuteCallback(ExecuteCallback callback) { executeCallback_ = std::move(callback); }
    void setTestCallback(TestCallback callback) { testCallback_ = std::move(callback); }
    void setTimeout(double ms) { timeout_ = ms; }
    void setFeatureFilter(std::vector<std::string> features) { enabledFeatures_ = std::move(features); }
    
    // =========================================================================
    // Running Tests
    // =========================================================================
    
    /**
     * @brief Run single test file
     */
    TestOutcome runTest(const std::filesystem::path& testFile);
    
    /**
     * @brief Run all tests in directory
     */
    std::vector<TestOutcome> runDirectory(const std::filesystem::path& dir);
    
    /**
     * @brief Run tests matching pattern
     */
    std::vector<TestOutcome> runPattern(const std::string& pattern);
    
    /**
     * @brief Run all Test262 tests
     */
    std::vector<TestOutcome> runAll();
    
    // =========================================================================
    // Results
    // =========================================================================
    
    struct Summary {
        size_t total = 0;
        size_t passed = 0;
        size_t failed = 0;
        size_t skipped = 0;
        size_t errors = 0;
        double totalTime = 0;
        
        double passRate() const {
            return total > 0 ? (100.0 * passed / (total - skipped)) : 0;
        }
    };
    
    Summary getSummary() const { return summary_; }
    const std::vector<TestOutcome>& getResults() const { return results_; }
    
    // =========================================================================
    // Utilities
    // =========================================================================
    
    /**
     * @brief Parse test metadata from YAML frontmatter
     */
    static TestMetadata parseMetadata(const std::string& source);
    
    /**
     * @brief Load harness file
     */
    std::string loadHarnessFile(const std::string& name);
    
private:
    std::filesystem::path test262Path_;
    std::filesystem::path harnessPath_;
    ExecuteCallback executeCallback_;
    TestCallback testCallback_;
    double timeout_ = 10000;  // 10 seconds default
    std::vector<std::string> enabledFeatures_;
    
    Summary summary_;
    std::vector<TestOutcome> results_;
    std::unordered_map<std::string, std::string> harnessCache_;
    HarnessGlobals globals_;
    
    bool shouldSkip(const TestMetadata& meta);
    std::string prepareTestCode(const std::string& source, const TestMetadata& meta);
    TestOutcome executeTest(const std::string& code, const TestMetadata& meta, 
                             const std::string& path);
};

// =============================================================================
// Feature Detection
// =============================================================================

namespace Features {

/**
 * @brief Get all supported features
 */
std::vector<std::string> supported();

/**
 * @brief Check if feature is supported
 */
bool isSupported(const std::string& feature);

/**
 * @brief Feature categories
 */
const std::vector<std::string> ES2024_FEATURES = {
    "ArrayBuffer", "Atomics", "BigInt", "DataView",
    "FinalizationRegistry", "Map", "Promise", "Proxy",
    "Reflect", "Set", "SharedArrayBuffer", "Symbol",
    "TypedArray", "WeakMap", "WeakRef", "WeakSet",
    "async-functions", "async-iteration", "class",
    "destructuring-binding", "for-of", "generators",
    "let", "new.target", "optional-chaining", "rest-parameters",
    "spread", "template", "Temporal", "Intl"
};

} // namespace Features

} // namespace Zepra::Spec
