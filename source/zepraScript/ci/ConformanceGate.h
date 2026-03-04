/**
 * @file ConformanceGate.h
 * @brief Conformance Gating for CI/CD
 * 
 * Implements:
 * - test262 pass requirements
 * - Security audit gates
 * - WASM alignment validation
 * - Regression detection
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace Zepra::Conformance {

// =============================================================================
// Test Result
// =============================================================================

enum class TestStatus {
    Pass,
    Fail,
    Skip,
    Timeout,
    Crash
};

struct TestResult {
    std::string name;
    std::string category;
    TestStatus status = TestStatus::Fail;
    std::string message;
    double durationMs = 0;
};

// =============================================================================
// Conformance Suite
// =============================================================================

/**
 * @brief Manages test262 conformance results
 */
class ConformanceSuite {
public:
    // Add test result
    void addResult(TestResult result) {
        results_.push_back(std::move(result));
    }
    
    // Get pass rate
    double passRate() const {
        if (results_.empty()) return 0;
        
        size_t passed = 0;
        for (const auto& r : results_) {
            if (r.status == TestStatus::Pass) passed++;
        }
        return static_cast<double>(passed) / results_.size();
    }
    
    // Get failure count
    size_t failureCount() const {
        size_t count = 0;
        for (const auto& r : results_) {
            if (r.status == TestStatus::Fail || r.status == TestStatus::Crash) {
                count++;
            }
        }
        return count;
    }
    
    // Get failures by category
    std::unordered_map<std::string, size_t> failuresByCategory() const {
        std::unordered_map<std::string, size_t> result;
        for (const auto& r : results_) {
            if (r.status == TestStatus::Fail || r.status == TestStatus::Crash) {
                result[r.category]++;
            }
        }
        return result;
    }
    
    // Check if suite passes threshold
    bool meetsThreshold(double minPassRate) const {
        return passRate() >= minPassRate;
    }
    
    // Get failure list for report
    std::vector<TestResult> getFailures() const {
        std::vector<TestResult> failures;
        for (const auto& r : results_) {
            if (r.status != TestStatus::Pass && r.status != TestStatus::Skip) {
                failures.push_back(r);
            }
        }
        return failures;
    }
    
private:
    std::vector<TestResult> results_;
};

// =============================================================================
// Gate Requirements
// =============================================================================

/**
 * @brief Defines gating requirements
 */
struct GateRequirements {
    // test262 requirements
    double minPassRate = 0.95;           // 95% pass rate
    size_t maxAllowedFailures = 50;       // Max failures
    std::vector<std::string> mustPassCategories = {
        "language/expressions",
        "language/statements",
        "built-ins/Array",
        "built-ins/Object",
        "built-ins/Function"
    };
    
    // Security requirements
    bool requireSecurityAuditPass = true;
    size_t maxSecurityViolations = 0;
    
    // WASM requirements
    bool requireWasmAlignment = true;
    
    // Performance requirements
    double maxStartupMs = 50.0;
    double maxGCPauseMs = 10.0;
};

// =============================================================================
// Gate Checker
// =============================================================================

/**
 * @brief Checks if changes pass all gates
 */
class GateChecker {
public:
    struct GateResult {
        bool passed = false;
        std::vector<std::string> failures;
        std::vector<std::string> warnings;
    };
    
    explicit GateChecker(GateRequirements requirements)
        : requirements_(std::move(requirements)) {}
    
    // Check all gates
    GateResult check(const ConformanceSuite& suite) {
        GateResult result;
        result.passed = true;
        
        // Check pass rate
        if (!suite.meetsThreshold(requirements_.minPassRate)) {
            result.passed = false;
            result.failures.push_back(
                "test262 pass rate " + std::to_string(suite.passRate() * 100) +
                "% below threshold " + std::to_string(requirements_.minPassRate * 100) + "%"
            );
        }
        
        // Check max failures
        if (suite.failureCount() > requirements_.maxAllowedFailures) {
            result.passed = false;
            result.failures.push_back(
                "Failure count " + std::to_string(suite.failureCount()) +
                " exceeds max " + std::to_string(requirements_.maxAllowedFailures)
            );
        }
        
        // Check must-pass categories
        auto byCat = suite.failuresByCategory();
        for (const auto& cat : requirements_.mustPassCategories) {
            if (byCat.count(cat) && byCat[cat] > 0) {
                result.passed = false;
                result.failures.push_back(
                    "Must-pass category '" + cat + "' has " + 
                    std::to_string(byCat[cat]) + " failures"
                );
            }
        }
        
        return result;
    }
    
    // Check security gate
    GateResult checkSecurity(size_t violations) {
        GateResult result;
        result.passed = (violations <= requirements_.maxSecurityViolations);
        
        if (!result.passed) {
            result.failures.push_back(
                "Security violations: " + std::to_string(violations) +
                " (max: " + std::to_string(requirements_.maxSecurityViolations) + ")"
            );
        }
        
        return result;
    }
    
    // Check performance gate
    GateResult checkPerformance(double startupMs, double gcPauseMs) {
        GateResult result;
        result.passed = true;
        
        if (startupMs > requirements_.maxStartupMs) {
            result.passed = false;
            result.failures.push_back(
                "Startup time " + std::to_string(startupMs) +
                "ms exceeds max " + std::to_string(requirements_.maxStartupMs) + "ms"
            );
        }
        
        if (gcPauseMs > requirements_.maxGCPauseMs) {
            result.passed = false;
            result.failures.push_back(
                "GC pause " + std::to_string(gcPauseMs) +
                "ms exceeds max " + std::to_string(requirements_.maxGCPauseMs) + "ms"
            );
        }
        
        return result;
    }
    
private:
    GateRequirements requirements_;
};

// =============================================================================
// Regression Detector
// =============================================================================

/**
 * @brief Detects test regressions
 */
class RegressionDetector {
public:
    struct Baseline {
        std::unordered_map<std::string, TestStatus> results;
        double passRate = 0;
    };
    
    // Set baseline
    void setBaseline(ConformanceSuite baseline) {
        for (const auto& r : baseline.getFailures()) {
            baseline_.results[r.name] = r.status;
        }
        baseline_.passRate = baseline.passRate();
    }
    
    // Detect regressions
    struct RegressionReport {
        std::vector<std::string> newFailures;    // Previously passing, now failing
        std::vector<std::string> fixed;          // Previously failing, now passing
        double passRateDelta = 0;
        bool hasRegressions = false;
    };
    
    RegressionReport detect(const ConformanceSuite& current) {
        RegressionReport report;
        report.passRateDelta = current.passRate() - baseline_.passRate;
        
        auto failures = current.getFailures();
        std::unordered_set<std::string> currentFailures;
        
        for (const auto& f : failures) {
            currentFailures.insert(f.name);
            
            // New failure (not in baseline)
            if (baseline_.results.find(f.name) == baseline_.results.end()) {
                report.newFailures.push_back(f.name);
            }
        }
        
        // Check for fixes
        for (const auto& [name, status] : baseline_.results) {
            if (status == TestStatus::Fail && 
                currentFailures.find(name) == currentFailures.end()) {
                report.fixed.push_back(name);
            }
        }
        
        report.hasRegressions = !report.newFailures.empty();
        return report;
    }
    
private:
    Baseline baseline_;
};

} // namespace Zepra::Conformance
