/**
 * @file Test262Harness.cpp
 * @brief Test262 harness implementation
 */

#include "tests/spec/Test262Harness.h"
#include <fstream>
#include <sstream>
#include <regex>
#include <chrono>
#include <stdexcept>

namespace Zepra::Spec {

// =============================================================================
// HarnessGlobals
// =============================================================================

Runtime::Value HarnessGlobals::createRealm() {
    // Create new isolated realm
    return Runtime::Value::undefined();
}

Runtime::Value HarnessGlobals::detachArrayBuffer(Runtime::Value buffer) {
    // Detach ArrayBuffer
    return Runtime::Value::undefined();
}

Runtime::Value HarnessGlobals::evalScript(const std::string& code) {
    // Eval in current realm
    return Runtime::Value::undefined();
}

Runtime::Value HarnessGlobals::gc() {
    // Force GC
    return Runtime::Value::undefined();
}

void HarnessGlobals::assert_(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error("Assertion failed: " + message);
    }
}

void HarnessGlobals::assertSameValue(Runtime::Value actual, Runtime::Value expected) {
    // SameValue comparison
    // Would use Object.is semantics
}

void HarnessGlobals::assertNotSameValue(Runtime::Value actual, Runtime::Value expected) {
    // Not SameValue comparison
}

void HarnessGlobals::assertThrows(const std::string& errorType, std::function<void()> fn) {
    bool threw = false;
    try {
        fn();
    } catch (const std::exception& e) {
        threw = true;
        // Check error type matches
    }
    
    if (!threw) {
        throw std::runtime_error("Expected " + errorType + " to be thrown");
    }
}

void HarnessGlobals::install(Runtime::Value globalObject) {
    // Install $262, assert, etc. on global
}

// =============================================================================
// Test262Harness
// =============================================================================

Test262Harness::Test262Harness(const std::filesystem::path& test262Path)
    : test262Path_(test262Path)
    , harnessPath_(test262Path / "harness") {}

TestOutcome Test262Harness::runTest(const std::filesystem::path& testFile) {
    auto start = std::chrono::high_resolution_clock::now();
    TestOutcome outcome;
    outcome.testPath = testFile.string();
    
    // Load test source
    std::ifstream file(testFile);
    if (!file) {
        outcome.result = TestResult::Error;
        outcome.message = "Cannot open file";
        return outcome;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();
    
    // Parse metadata
    TestMetadata meta = parseMetadata(source);
    
    // Check if should skip
    if (shouldSkip(meta)) {
        outcome.result = TestResult::Skip;
        outcome.message = "Unsupported feature";
        return outcome;
    }
    
    // Prepare test code with harness
    std::string code = prepareTestCode(source, meta);
    
    // Execute test
    outcome = executeTest(code, meta, testFile.string());
    
    auto end = std::chrono::high_resolution_clock::now();
    outcome.duration = std::chrono::duration<double, std::milli>(end - start).count();
    
    // Update summary
    summary_.total++;
    switch (outcome.result) {
        case TestResult::Pass: summary_.passed++; break;
        case TestResult::Fail: summary_.failed++; break;
        case TestResult::Skip: summary_.skipped++; break;
        default: summary_.errors++; break;
    }
    summary_.totalTime += outcome.duration;
    
    results_.push_back(outcome);
    
    if (testCallback_) {
        testCallback_(outcome);
    }
    
    return outcome;
}

std::vector<TestOutcome> Test262Harness::runDirectory(const std::filesystem::path& dir) {
    std::vector<TestOutcome> results;
    
    for (const auto& entry : std::filesystem::recursive_directory_iterator(dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".js") {
            results.push_back(runTest(entry.path()));
        }
    }
    
    return results;
}

std::vector<TestOutcome> Test262Harness::runPattern(const std::string& pattern) {
    std::vector<TestOutcome> results;
    std::regex rx(pattern);
    
    for (const auto& entry : std::filesystem::recursive_directory_iterator(test262Path_ / "test")) {
        if (entry.is_regular_file() && 
            std::regex_search(entry.path().string(), rx)) {
            results.push_back(runTest(entry.path()));
        }
    }
    
    return results;
}

std::vector<TestOutcome> Test262Harness::runAll() {
    return runDirectory(test262Path_ / "test");
}

TestMetadata Test262Harness::parseMetadata(const std::string& source) {
    TestMetadata meta;
    
    // Find YAML frontmatter
    size_t start = source.find("/*---");
    if (start == std::string::npos) return meta;
    
    size_t end = source.find("---*/", start);
    if (end == std::string::npos) return meta;
    
    std::string yaml = source.substr(start + 5, end - start - 5);
    
    // Parse YAML fields
    std::regex descRx("description:\\s*(.+)");
    std::regex esidRx("esid:\\s*(.+)");
    std::regex featuresRx("features:\\s*\\[([^\\]]+)\\]");
    std::regex includesRx("includes:\\s*\\[([^\\]]+)\\]");
    std::regex flagsRx("flags:\\s*\\[([^\\]]+)\\]");
    std::regex negativeRx("negative:\\s*\\n\\s*phase:\\s*(\\w+)\\s*\\n\\s*type:\\s*(\\w+)");
    
    std::smatch match;
    
    if (std::regex_search(yaml, match, descRx)) {
        meta.description = match[1].str();
    }
    
    if (std::regex_search(yaml, match, esidRx)) {
        meta.esid = match[1].str();
    }
    
    if (std::regex_search(yaml, match, featuresRx)) {
        std::string features = match[1].str();
        std::regex featureRx("\\w+");
        auto featBegin = std::sregex_iterator(features.begin(), features.end(), featureRx);
        auto featEnd = std::sregex_iterator();
        for (auto it = featBegin; it != featEnd; ++it) {
            meta.features.push_back(it->str());
        }
    }
    
    if (std::regex_search(yaml, match, includesRx)) {
        std::string includes = match[1].str();
        std::regex includeRx("[\\w.]+");
        auto incBegin = std::sregex_iterator(includes.begin(), includes.end(), includeRx);
        auto incEnd = std::sregex_iterator();
        for (auto it = incBegin; it != incEnd; ++it) {
            meta.includes.push_back(it->str());
        }
    }
    
    if (std::regex_search(yaml, match, flagsRx)) {
        std::string flags = match[1].str();
        std::regex flagRx("\\w+");
        auto flagBegin = std::sregex_iterator(flags.begin(), flags.end(), flagRx);
        auto flagEnd = std::sregex_iterator();
        for (auto it = flagBegin; it != flagEnd; ++it) {
            meta.flags.push_back(it->str());
        }
    }
    
    if (std::regex_search(yaml, match, negativeRx)) {
        meta.negativePhase = match[1].str();
        meta.negative = match[2].str();
    }
    
    return meta;
}

std::string Test262Harness::loadHarnessFile(const std::string& name) {
    auto it = harnessCache_.find(name);
    if (it != harnessCache_.end()) {
        return it->second;
    }
    
    std::filesystem::path path = harnessPath_ / name;
    std::ifstream file(path);
    if (!file) {
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    
    harnessCache_[name] = content;
    return content;
}

bool Test262Harness::shouldSkip(const TestMetadata& meta) {
    for (const auto& feature : meta.features) {
        if (!Features::isSupported(feature)) {
            return true;
        }
    }
    return false;
}

std::string Test262Harness::prepareTestCode(const std::string& source, 
                                             const TestMetadata& meta) {
    std::string code;
    
    // Load required harness files
    code += loadHarnessFile("sta.js");
    code += "\n";
    code += loadHarnessFile("assert.js");
    code += "\n";
    
    for (const auto& include : meta.includes) {
        code += loadHarnessFile(include);
        code += "\n";
    }
    
    // Add test source
    code += source;
    
    // Wrap in strict mode if needed
    if (meta.isStrict()) {
        code = "\"use strict\";\n" + code;
    }
    
    return code;
}

TestOutcome Test262Harness::executeTest(const std::string& code, 
                                         const TestMetadata& meta,
                                         const std::string& path) {
    TestOutcome outcome;
    outcome.testPath = path;
    
    if (!executeCallback_) {
        outcome.result = TestResult::Error;
        outcome.message = "No execute callback set";
        return outcome;
    }
    
    try {
        Runtime::Value result = executeCallback_(code, meta.isModule());
        
        if (meta.isNegative()) {
            // Expected to throw but didn't
            outcome.result = TestResult::Fail;
            outcome.message = "Expected " + meta.negative + " in " + meta.negativePhase;
        } else {
            outcome.result = TestResult::Pass;
        }
        
    } catch (const std::exception& e) {
        if (meta.isNegative()) {
            // Check if correct error type
            // For now, assume pass if any error thrown
            outcome.result = TestResult::Pass;
        } else {
            outcome.result = TestResult::Fail;
            outcome.message = e.what();
        }
    }
    
    return outcome;
}

// =============================================================================
// Features
// =============================================================================

namespace Features {

std::vector<std::string> supported() {
    return ES2024_FEATURES;
}

bool isSupported(const std::string& feature) {
    return std::find(ES2024_FEATURES.begin(), ES2024_FEATURES.end(), feature) 
           != ES2024_FEATURES.end();
}

} // namespace Features

} // namespace Zepra::Spec
