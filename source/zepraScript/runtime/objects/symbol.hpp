#pragma once

/**
 * @file symbol.hpp
 * @brief Symbol builtin implementation
 */

#include "config.hpp"
#include "value.hpp"
#include <string>
#include <unordered_map>
#include <atomic>

namespace Zepra::Runtime {

/**
 * @brief Symbol registry (global)
 * 
 * Manages:
 * - Unique symbol IDs
 * - Symbol.for() global registry
 * - Well-known symbols (Symbol.iterator, etc.)
 */
class SymbolRegistry {
public:
    static SymbolRegistry& instance();
    
    // Create new unique symbol
    Value createSymbol(const std::string& description = "");
    
    // Global symbol registry (Symbol.for)
    Value symbolFor(const std::string& key);
    std::string symbolKeyFor(Value symbol);
    
    // Well-known symbols
    Value getIterator() const { return iterator_; }
    Value getToStringTag() const { return toStringTag_; }
    Value getHasInstance() const { return hasInstance_; }
    Value getIsConcatSpreadable() const { return isConcatSpreadable_; }
    Value getUnscopables() const { return unscopables_; }
    
    // Get description for symbol ID
    std::string getDescription(uint32_t id) const;
    
private:
    SymbolRegistry();
    
    std::atomic<uint32_t> nextId_{1};  // 0 reserved
    std::unordered_map<std::string, std::string> descriptions_;
    std::unordered_map<std::string, Value> globalRegistry_;  // Symbol.for() registry
    
    // Well-known symbols
    Value iterator_;
    Value toStringTag_;
    Value hasInstance_;
    Value isConcatSpreadable_;
    Value unscopables_;
};

} // namespace Zepra::Runtime
