#include "runtime/objects/symbol.hpp"
#include <mutex>

namespace Zepra::Runtime {

static std::mutex registry_mutex;

SymbolRegistry& SymbolRegistry::instance() {
    static SymbolRegistry instance;
    return instance;
}

SymbolRegistry::SymbolRegistry() {
    // Create well-known symbols
    iterator_ = Value::symbol(nextId_++);
    descriptions_[std::to_string(iterator_.asSymbol())] = "Symbol.iterator";
    
    toStringTag_ = Value::symbol(nextId_++);
    descriptions_[std::to_string(toStringTag_.asSymbol())] = "Symbol.toStringTag";
    
    hasInstance_ = Value::symbol(nextId_++);
    descriptions_[std::to_string(hasInstance_.asSymbol())] = "Symbol.hasInstance";
    
    isConcatSpreadable_ = Value::symbol(nextId_++);
    descriptions_[std::to_string(isConcatSpreadable_.asSymbol())] = "Symbol.isConcatSpreadable";
    
    unscopables_ = Value::symbol(nextId_++);
    descriptions_[std::to_string(unscopables_.asSymbol())] = "Symbol.unscopables";
}

Value SymbolRegistry::createSymbol(const std::string& description) {
    std::lock_guard<std::mutex> lock(registry_mutex);
    
    uint32_t id = nextId_++;
    Value symbol = Value::symbol(id);
    
    if (!description.empty()) {
        descriptions_[std::to_string(id)] = description;
    }
    
    return symbol;
}

Value SymbolRegistry::symbolFor(const std::string& key) {
    std::lock_guard<std::mutex> lock(registry_mutex);
    
    auto it = globalRegistry_.find(key);
    if (it != globalRegistry_.end()) {
        return it->second;
    }
    
    // Create new symbol for this key
    uint32_t id = nextId_++;
    Value symbol = Value::symbol(id);
    descriptions_[std::to_string(id)] = key;
    globalRegistry_[key] = symbol;
    
    return symbol;
}

std::string SymbolRegistry::symbolKeyFor(Value symbol) {
    if (!symbol.isSymbol()) {
        return "";
    }
    
    std::lock_guard<std::mutex> lock(registry_mutex);
    
    // Search global registry
    for (const auto& pair : globalRegistry_) {
        if (pair.second.asSymbol() == symbol.asSymbol()) {
            return pair.first;
        }
    }
    
    return "";  // Not a global symbol
}

std::string SymbolRegistry::getDescription(uint32_t id) const {
    auto it = descriptions_.find(std::to_string(id));
    if (it != descriptions_.end()) {
        return it->second;
    }
    return "";
}

} // namespace Zepra::Runtime
