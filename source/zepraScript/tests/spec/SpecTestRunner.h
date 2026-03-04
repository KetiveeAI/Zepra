/**
 * @file SpecTestRunner.h
 * @brief WebAssembly Spec Test Harness
 * 
 * Runs official WASM spec tests defined in JSON format.
 * - Parses test commands (module, assert_return, assert_trap)
 * - Executes tests against ZepraScript WASM engine
 * - Verifies results
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <variant>

#include "wasm/WasmModule.h"
#include "wasm/WasmInstance.h"
#include "runtime/objects/value.hpp"

namespace Zepra::Wasm::Test {

// Test command types
enum class CommandType {
    Module,
    AssertReturn,
    AssertTrap,
    AssertMalformed,
    AssertInvalid,
    AssertUninstantiable,
    Register,
    Action
};

// Value type for test arguments/results
struct TestValue {
    enum Type { Copy, RefNull, RefFunc, RefExtern } type;
    Value value;
};

// Action to perform (invoke, get)
struct Action {
    enum Type { Invoke, Get } type;
    std::string moduleName; // Optional
    std::string fieldName;
    std::vector<TestValue> args;
};

// Complete test command
struct Command {
    CommandType type;
    int line;
    std::string filename;
    
    // For Module
    std::string name;
    std::vector<uint8_t> wasm;
    
    // For Assert/Action
    Action action;
    std::vector<TestValue> expected;
    std::string expectedText; // For assert_trap/malformed
};

class SpecTestRunner {
public:
    SpecTestRunner();
    ~SpecTestRunner();
    
    // Run a single JSON test file
    bool runFile(const std::string& path);
    
    // Register a named module (for imports)
    void registerModule(const std::string& name, std::shared_ptr<WasmInstance> instance);
    
    // Statistics
    int passed() const { return passed_; }
    int failed() const { return failed_; }
    int skipped() const { return skipped_; }

private:
    bool executeCommand(const Command& cmd);
    bool runAssertReturn(const Command& cmd);
    bool runAssertTrap(const Command& cmd);
    bool runModule(const Command& cmd);
    
    Value executeAction(const Action& action);
    
    // Helper to parse JSON (using simple internal parser or rapidjson)
    std::vector<Command> parseJson(const std::string& jsonContent);
    
    // State
    std::shared_ptr<WasmInstance> currentInstance_;
    std::unordered_map<std::string, std::shared_ptr<WasmInstance>> namedInstances_;
    
    int passed_ = 0;
    int failed_ = 0;
    int skipped_ = 0;
};

} // namespace Zepra::Wasm::Test
