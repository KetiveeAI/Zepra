/**
 * @file SymbolAPI.h
 * @brief Symbol Implementation
 * 
 * ECMAScript Symbol based on:
 * - JSC Symbol.h
 * - ECMA-262 6.1.7 Symbol Type
 */

#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <optional>

namespace Zepra::Runtime {

// =============================================================================
// Symbol
// =============================================================================

class Symbol {
public:
    using SymbolId = uint64_t;
    
    explicit Symbol(std::optional<std::string> description = std::nullopt)
        : id_(nextId())
        , description_(std::move(description)) {}
    
    SymbolId id() const { return id_; }
    
    bool hasDescription() const { return description_.has_value(); }
    const std::string& description() const { return *description_; }
    std::optional<std::string> optionalDescription() const { return description_; }
    
    std::string toString() const {
        if (description_) {
            return "Symbol(" + *description_ + ")";
        }
        return "Symbol()";
    }
    
    bool operator==(const Symbol& other) const { return id_ == other.id_; }
    bool operator!=(const Symbol& other) const { return id_ != other.id_; }
    
private:
    static SymbolId nextId() {
        static std::atomic<SymbolId> counter{1};
        return counter.fetch_add(1, std::memory_order_relaxed);
    }
    
    SymbolId id_;
    std::optional<std::string> description_;
};

// =============================================================================
// Global Symbol Registry
// =============================================================================

class SymbolRegistry {
public:
    static SymbolRegistry& global() {
        static SymbolRegistry instance;
        return instance;
    }
    
    std::shared_ptr<Symbol> for_(const std::string& key) {
        std::lock_guard lock(mutex_);
        
        auto it = keyToSymbol_.find(key);
        if (it != keyToSymbol_.end()) {
            return it->second;
        }
        
        auto symbol = std::make_shared<Symbol>(key);
        keyToSymbol_[key] = symbol;
        symbolToKey_[symbol->id()] = key;
        return symbol;
    }
    
    std::optional<std::string> keyFor(const std::shared_ptr<Symbol>& symbol) const {
        std::lock_guard lock(mutex_);
        
        auto it = symbolToKey_.find(symbol->id());
        if (it != symbolToKey_.end()) {
            return it->second;
        }
        return std::nullopt;
    }
    
    bool isRegistered(const std::shared_ptr<Symbol>& symbol) const {
        std::lock_guard lock(mutex_);
        return symbolToKey_.find(symbol->id()) != symbolToKey_.end();
    }
    
private:
    SymbolRegistry() = default;
    
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::shared_ptr<Symbol>> keyToSymbol_;
    std::unordered_map<Symbol::SymbolId, std::string> symbolToKey_;
};

// =============================================================================
// Well-Known Symbols
// =============================================================================

class WellKnownSymbols {
public:
    static WellKnownSymbols& instance() {
        static WellKnownSymbols inst;
        return inst;
    }
    
    std::shared_ptr<Symbol> asyncIterator() const { return asyncIterator_; }
    std::shared_ptr<Symbol> hasInstance() const { return hasInstance_; }
    std::shared_ptr<Symbol> isConcatSpreadable() const { return isConcatSpreadable_; }
    std::shared_ptr<Symbol> iterator() const { return iterator_; }
    std::shared_ptr<Symbol> match() const { return match_; }
    std::shared_ptr<Symbol> matchAll() const { return matchAll_; }
    std::shared_ptr<Symbol> replace() const { return replace_; }
    std::shared_ptr<Symbol> search() const { return search_; }
    std::shared_ptr<Symbol> species() const { return species_; }
    std::shared_ptr<Symbol> split() const { return split_; }
    std::shared_ptr<Symbol> toPrimitive() const { return toPrimitive_; }
    std::shared_ptr<Symbol> toStringTag() const { return toStringTag_; }
    std::shared_ptr<Symbol> unscopables() const { return unscopables_; }
    std::shared_ptr<Symbol> dispose() const { return dispose_; }
    std::shared_ptr<Symbol> asyncDispose() const { return asyncDispose_; }
    
private:
    WellKnownSymbols()
        : asyncIterator_(std::make_shared<Symbol>("Symbol.asyncIterator"))
        , hasInstance_(std::make_shared<Symbol>("Symbol.hasInstance"))
        , isConcatSpreadable_(std::make_shared<Symbol>("Symbol.isConcatSpreadable"))
        , iterator_(std::make_shared<Symbol>("Symbol.iterator"))
        , match_(std::make_shared<Symbol>("Symbol.match"))
        , matchAll_(std::make_shared<Symbol>("Symbol.matchAll"))
        , replace_(std::make_shared<Symbol>("Symbol.replace"))
        , search_(std::make_shared<Symbol>("Symbol.search"))
        , species_(std::make_shared<Symbol>("Symbol.species"))
        , split_(std::make_shared<Symbol>("Symbol.split"))
        , toPrimitive_(std::make_shared<Symbol>("Symbol.toPrimitive"))
        , toStringTag_(std::make_shared<Symbol>("Symbol.toStringTag"))
        , unscopables_(std::make_shared<Symbol>("Symbol.unscopables"))
        , dispose_(std::make_shared<Symbol>("Symbol.dispose"))
        , asyncDispose_(std::make_shared<Symbol>("Symbol.asyncDispose")) {}
    
    std::shared_ptr<Symbol> asyncIterator_;
    std::shared_ptr<Symbol> hasInstance_;
    std::shared_ptr<Symbol> isConcatSpreadable_;
    std::shared_ptr<Symbol> iterator_;
    std::shared_ptr<Symbol> match_;
    std::shared_ptr<Symbol> matchAll_;
    std::shared_ptr<Symbol> replace_;
    std::shared_ptr<Symbol> search_;
    std::shared_ptr<Symbol> species_;
    std::shared_ptr<Symbol> split_;
    std::shared_ptr<Symbol> toPrimitive_;
    std::shared_ptr<Symbol> toStringTag_;
    std::shared_ptr<Symbol> unscopables_;
    std::shared_ptr<Symbol> dispose_;
    std::shared_ptr<Symbol> asyncDispose_;
};

// =============================================================================
// Symbol Factory Functions
// =============================================================================

inline std::shared_ptr<Symbol> createSymbol(const std::string& description) {
    return std::make_shared<Symbol>(description);
}

inline std::shared_ptr<Symbol> createSymbol() {
    return std::make_shared<Symbol>();
}

inline std::shared_ptr<Symbol> symbolFor(const std::string& key) {
    return SymbolRegistry::global().for_(key);
}

inline std::optional<std::string> symbolKeyFor(const std::shared_ptr<Symbol>& symbol) {
    return SymbolRegistry::global().keyFor(symbol);
}

} // namespace Zepra::Runtime
