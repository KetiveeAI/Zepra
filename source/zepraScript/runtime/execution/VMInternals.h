/**
 * @file VMInternals.h
 * @brief Virtual machine internal structures and helpers
 * 
 * Implements:
 * - VM configuration
 * - Runtime helpers
 * - Property access helpers
 * - Type conversion utilities
 * 
 * Internal API for VM subsystems
 */

#pragma once

#include "runtime/objects/value.hpp"
#include "runtime/objects/object.hpp"
#include "heap/gc_heap.hpp"
#include <functional>
#include <chrono>

namespace Zepra::VM {

// =============================================================================
// VM Configuration
// =============================================================================

struct VMConfig {
    // Memory
    size_t initialHeapSize = 16 * 1024 * 1024;      // 16MB
    size_t maxHeapSize = 512 * 1024 * 1024;         // 512MB
    size_t nurserySize = 4 * 1024 * 1024;           // 4MB
    
    // Stack
    size_t maxStackSize = 1024 * 1024;              // 1MB
    size_t maxCallDepth = 10000;
    
    // JIT
    bool enableJIT = true;
    size_t jitThreshold = 1000;                     // Calls before JIT
    size_t inlineThreshold = 100;                   // Bytecode size for inlining
    
    // GC
    bool enableConcurrentGC = true;
    double gcHeapGrowthFactor = 2.0;
    
    // Debug
    bool enableDebugger = false;
    bool enableProfiler = false;
    bool enableTracing = false;
};

// =============================================================================
// Runtime Statistics
// =============================================================================

struct VMStats {
    // Execution
    size_t bytecodeExecuted = 0;
    size_t callsExecuted = 0;
    size_t exceptionsThrown = 0;
    
    // Compilation
    size_t functionsCompiled = 0;
    size_t bytesCompiled = 0;
    size_t jitCompilations = 0;
    size_t deoptimizations = 0;
    
    // Memory
    size_t totalAllocated = 0;
    size_t peakHeapSize = 0;
    size_t gcCycles = 0;
    std::chrono::milliseconds totalGCTime{0};
    
    // Timing
    std::chrono::milliseconds parseTime{0};
    std::chrono::milliseconds compileTime{0};
    std::chrono::milliseconds executeTime{0};
};

// =============================================================================
// Property Access Helpers
// =============================================================================

namespace PropertyAccess {

/**
 * @brief Get property with prototype chain lookup
 */
Runtime::Value get(Runtime::Object* obj, const std::string& name);

/**
 * @brief Get property by index (array access)
 */
Runtime::Value getIndex(Runtime::Object* obj, size_t index);

/**
 * @brief Set property
 */
bool set(Runtime::Object* obj, const std::string& name, 
         Runtime::Value value, bool strict = false);

/**
 * @brief Set property by index
 */
bool setIndex(Runtime::Object* obj, size_t index, 
              Runtime::Value value, bool strict = false);

/**
 * @brief Delete property
 */
bool del(Runtime::Object* obj, const std::string& name, bool strict = false);

/**
 * @brief Has own property
 */
bool hasOwn(Runtime::Object* obj, const std::string& name);

/**
 * @brief Has property (with prototype chain)
 */
bool has(Runtime::Object* obj, const std::string& name);

} // namespace PropertyAccess

// =============================================================================
// Type Conversion Helpers
// =============================================================================

namespace Convert {

/**
 * @brief ToNumber (ES spec 7.1.3)
 */
double toNumber(Runtime::Value value);

/**
 * @brief ToInteger (ES spec 7.1.4) - deprecated, use ToIntegerOrInfinity
 */
int64_t toInteger(Runtime::Value value);

/**
 * @brief ToString (ES spec 7.1.17)
 */
std::string toString(Runtime::Value value);

/**
 * @brief ToBoolean (ES spec 7.1.2)
 */
bool toBoolean(Runtime::Value value);

/**
 * @brief ToObject (ES spec 7.1.18)
 */
Runtime::Object* toObject(Runtime::Value value, GC::GCController& gc);

/**
 * @brief ToPrimitive (ES spec 7.1.1)
 */
Runtime::Value toPrimitive(Runtime::Value value, 
                           const std::string& preferredType = "");

/**
 * @brief ToPropertyKey (ES spec 7.1.19)
 */
std::string toPropertyKey(Runtime::Value value);

/**
 * @brief ToInt32 (ES spec 7.1.5)
 */
int32_t toInt32(Runtime::Value value);

/**
 * @brief ToUint32 (ES spec 7.1.6)
 */
uint32_t toUint32(Runtime::Value value);

/**
 * @brief ToLength (ES spec 7.1.15)
 */
size_t toLength(Runtime::Value value);

/**
 * @brief ToIndex (ES spec 7.1.22)
 */
size_t toIndex(Runtime::Value value);

} // namespace Convert

// =============================================================================
// Comparison Helpers
// =============================================================================

namespace Compare {

/**
 * @brief Abstract equality (==)
 */
bool abstractEqual(Runtime::Value x, Runtime::Value y);

/**
 * @brief Strict equality (===)
 */
bool strictEqual(Runtime::Value x, Runtime::Value y);

/**
 * @brief SameValue (Object.is)
 */
bool sameValue(Runtime::Value x, Runtime::Value y);

/**
 * @brief SameValueZero (used by Map, Set)
 */
bool sameValueZero(Runtime::Value x, Runtime::Value y);

/**
 * @brief Abstract relational comparison (<)
 */
Runtime::Value abstractRelational(Runtime::Value x, Runtime::Value y, 
                                  bool leftFirst = true);

} // namespace Compare

// =============================================================================
// Error Creation
// =============================================================================

namespace Errors {

Runtime::Value typeError(GC::GCController& gc, const std::string& message);
Runtime::Value referenceError(GC::GCController& gc, const std::string& message);
Runtime::Value syntaxError(GC::GCController& gc, const std::string& message);
Runtime::Value rangeError(GC::GCController& gc, const std::string& message);
Runtime::Value uriError(GC::GCController& gc, const std::string& message);
Runtime::Value evalError(GC::GCController& gc, const std::string& message);
Runtime::Value aggregateError(GC::GCController& gc, 
                              const std::vector<Runtime::Value>& errors,
                              const std::string& message);

} // namespace Errors

// =============================================================================
// Iteration Helpers
// =============================================================================

namespace Iteration {

/**
 * @brief Get iterator from object
 */
Runtime::Value getIterator(Runtime::Value obj, const std::string& method = "");

/**
 * @brief Call iterator.next()
 */
Runtime::Value iteratorNext(Runtime::Value iterator, Runtime::Value value = Runtime::Value::undefined());

/**
 * @brief Check if iterator result is done
 */
bool iteratorComplete(Runtime::Value iterResult);

/**
 * @brief Get value from iterator result
 */
Runtime::Value iteratorValue(Runtime::Value iterResult);

/**
 * @brief Close iterator (call return method if present)
 */
void iteratorClose(Runtime::Value iterator, bool completion = true);

} // namespace Iteration

} // namespace Zepra::VM
