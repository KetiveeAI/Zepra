/**
 * @file test_helpers.h
 * @brief Common Test Utilities for ZepraScript
 */

#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <chrono>
#include <functional>

// Include runtime APIs for testing
#include "runtime/builtins_api/ArrayAPI.h"
#include "runtime/builtins_api/StringAPI.h"
#include "runtime/objects/MapSetAPI.h"
#include "runtime/builtins_api/Base64API.h"
#include "runtime/objects/CoercionAPI.h"
#include <stdexcept>

namespace Zepra::Tests {

// =============================================================================
// Test Macros
// =============================================================================

#define ZEPRA_TEST(name) void test_##name()
#define ZEPRA_RUN_TEST(name) runTest(#name, test_##name)

// =============================================================================
// Assertions
// =============================================================================

inline void assertTrue(bool condition, const std::string& message = "") {
    if (!condition) {
        throw std::runtime_error("Assertion failed: " + message);
    }
}

inline void assertFalse(bool condition, const std::string& message = "") {
    assertTrue(!condition, message);
}

template<typename T>
void assertEqual(const T& expected, const T& actual, const std::string& message = "") {
    if (expected != actual) {
        throw std::runtime_error("assertEqual failed: " + message);
    }
}

template<typename T>
void assertNotEqual(const T& expected, const T& actual, const std::string& message = "") {
    if (expected == actual) {
        throw std::runtime_error("assertNotEqual failed: " + message);
    }
}

inline void assertNull(const void* ptr, const std::string& message = "") {
    if (ptr != nullptr) {
        throw std::runtime_error("assertNull failed: " + message);
    }
}

inline void assertNotNull(const void* ptr, const std::string& message = "") {
    if (ptr == nullptr) {
        throw std::runtime_error("assertNotNull failed: " + message);
    }
}

template<typename Exception, typename Fn>
void assertThrows(Fn fn, const std::string& message = "") {
    try {
        fn();
        throw std::runtime_error("Expected exception not thrown: " + message);
    } catch (const Exception&) {
        // Expected
    } catch (...) {
        throw std::runtime_error("Wrong exception type: " + message);
    }
}

// =============================================================================
// Test Runner
// =============================================================================

struct TestResult {
    std::string name;
    bool passed;
    std::string error;
    double durationMs;
};

class TestRunner {
public:
    void runTest(const std::string& name, std::function<void()> testFn) {
        auto start = std::chrono::high_resolution_clock::now();
        
        TestResult result;
        result.name = name;
        
        try {
            testFn();
            result.passed = true;
        } catch (const std::exception& e) {
            result.passed = false;
            result.error = e.what();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        result.durationMs = std::chrono::duration<double, std::milli>(end - start).count();
        
        results_.push_back(result);
    }
    
    void printSummary() const {
        int passed = 0, failed = 0;
        
        std::cout << "\n=== Test Results ===\n";
        
        for (const auto& result : results_) {
            if (result.passed) {
                std::cout << "[PASS] " << result.name << " (" << result.durationMs << "ms)\n";
                ++passed;
            } else {
                std::cout << "[FAIL] " << result.name << ": " << result.error << "\n";
                ++failed;
            }
        }
        
        std::cout << "\n=== Summary ===\n";
        std::cout << "Passed: " << passed << ", Failed: " << failed << ", Total: " << results_.size() << "\n";
    }
    
    bool allPassed() const {
        for (const auto& r : results_) {
            if (!r.passed) return false;
        }
        return true;
    }

private:
    std::vector<TestResult> results_;
};

// =============================================================================
// Performance Helpers
// =============================================================================

template<typename Fn>
double benchmark(Fn fn, int iterations = 1000) {
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        fn();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count() / iterations;
}

} // namespace Zepra::Tests
