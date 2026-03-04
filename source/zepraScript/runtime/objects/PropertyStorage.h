/**
 * @file PropertyStorage.h
 * @brief Efficient property storage for JS objects
 * 
 * Implements:
 * - Inline property storage
 * - Overflow property array
 * - Fast property access by offset
 * - Dictionary mode fallback
 * 
 * Based on V8 property storage
 */

#pragma once

#include "value.hpp"
#include "Shape.h"
#include <vector>
#include <unordered_map>
#include <array>

namespace Zepra::Runtime {

// =============================================================================
// Property Storage Mode
// =============================================================================

enum class PropertyMode : uint8_t {
    Fast,           // Shape-based, inline/overflow storage
    Dictionary      // Hash table (after too many changes)
};

// =============================================================================
// Inline Property Storage
// =============================================================================

/**
 * @brief Fixed-size inline storage for first N properties
 */
template<size_t N = 4>
class InlineProperties {
public:
    static constexpr size_t CAPACITY = N;
    
    Value get(size_t index) const {
        return index < N ? values_[index] : Value::undefined();
    }
    
    void set(size_t index, Value value) {
        if (index < N) {
            values_[index] = value;
        }
    }
    
    bool fitsInline(size_t count) const {
        return count <= N;
    }
    
private:
    std::array<Value, N> values_;
};

// =============================================================================
// Overflow Property Array
// =============================================================================

/**
 * @brief Dynamic storage for properties beyond inline capacity
 */
class OverflowProperties {
public:
    OverflowProperties() = default;
    explicit OverflowProperties(size_t capacity) {
        values_.reserve(capacity);
    }
    
    Value get(size_t index) const {
        return index < values_.size() ? values_[index] : Value::undefined();
    }
    
    void set(size_t index, Value value) {
        ensureCapacity(index + 1);
        values_[index] = value;
    }
    
    size_t size() const { return values_.size(); }
    size_t capacity() const { return values_.capacity(); }
    
    void resize(size_t newSize) {
        values_.resize(newSize, Value::undefined());
    }
    
private:
    std::vector<Value> values_;
    
    void ensureCapacity(size_t needed) {
        if (needed > values_.size()) {
            values_.resize(needed, Value::undefined());
        }
    }
};

// =============================================================================
// Dictionary Properties
// =============================================================================

/**
 * @brief Hash table storage for dictionary mode
 */
class DictionaryProperties {
public:
    struct Entry {
        Value value;
        PropertyAttributes attributes;
    };
    
    bool has(const std::string& name) const {
        return entries_.find(name) != entries_.end();
    }
    
    Value get(const std::string& name) const {
        auto it = entries_.find(name);
        return it != entries_.end() ? it->second.value : Value::undefined();
    }
    
    void set(const std::string& name, Value value, PropertyAttributes attrs = PropertyAttributes::Default) {
        entries_[name] = {value, attrs};
    }
    
    bool remove(const std::string& name) {
        return entries_.erase(name) > 0;
    }
    
    PropertyAttributes getAttributes(const std::string& name) const {
        auto it = entries_.find(name);
        return it != entries_.end() ? it->second.attributes : PropertyAttributes::None;
    }
    
    std::vector<std::string> keys() const {
        std::vector<std::string> result;
        result.reserve(entries_.size());
        for (const auto& [key, _] : entries_) {
            result.push_back(key);
        }
        return result;
    }
    
    size_t size() const { return entries_.size(); }
    
private:
    std::unordered_map<std::string, Entry> entries_;
};

// =============================================================================
// Property Storage
// =============================================================================

/**
 * @brief Complete property storage for an object
 */
class PropertyStorage {
public:
    static constexpr size_t INLINE_CAPACITY = 4;
    static constexpr size_t DICTIONARY_THRESHOLD = 32;  // Transitions before dictionary
    
    PropertyStorage() : mode_(PropertyMode::Fast) {}
    
    // =========================================================================
    // Mode
    // =========================================================================
    
    PropertyMode mode() const { return mode_; }
    bool isFastMode() const { return mode_ == PropertyMode::Fast; }
    bool isDictionaryMode() const { return mode_ == PropertyMode::Dictionary; }
    
    /**
     * @brief Convert to dictionary mode
     */
    void convertToDictionary(Shape* shape);
    
    // =========================================================================
    // Fast Mode Access
    // =========================================================================
    
    /**
     * @brief Get property by offset (fast path)
     */
    Value getByOffset(uint32_t offset) const {
        if (offset < INLINE_CAPACITY) {
            return inline_.get(offset);
        }
        return overflow_.get(offset - INLINE_CAPACITY);
    }
    
    /**
     * @brief Set property by offset (fast path)
     */
    void setByOffset(uint32_t offset, Value value) {
        if (offset < INLINE_CAPACITY) {
            inline_.set(offset, value);
        } else {
            overflow_.set(offset - INLINE_CAPACITY, value);
        }
    }
    
    // =========================================================================
    // Dictionary Mode Access
    // =========================================================================
    
    Value getDictionary(const std::string& name) const {
        return dictionary_.get(name);
    }
    
    void setDictionary(const std::string& name, Value value, 
                       PropertyAttributes attrs = PropertyAttributes::Default) {
        dictionary_.set(name, value, attrs);
    }
    
    bool hasDictionary(const std::string& name) const {
        return dictionary_.has(name);
    }
    
    bool removeDictionary(const std::string& name) {
        return dictionary_.remove(name);
    }
    
    // =========================================================================
    // Capacity
    // =========================================================================
    
    /**
     * @brief Ensure capacity for N properties
     */
    void ensureCapacity(size_t count) {
        if (count > INLINE_CAPACITY) {
            overflow_.resize(count - INLINE_CAPACITY);
        }
    }
    
    size_t propertyCapacity() const {
        return INLINE_CAPACITY + overflow_.capacity();
    }
    
    // =========================================================================
    // Statistics
    // =========================================================================
    
    struct Stats {
        PropertyMode mode;
        size_t inlineUsed;
        size_t overflowUsed;
        size_t dictionarySize;
    };
    
    Stats getStats() const {
        return {
            mode_,
            0, // Would need tracking
            overflow_.size(),
            isDictionaryMode() ? dictionary_.size() : 0
        };
    }
    
private:
    PropertyMode mode_;
    InlineProperties<INLINE_CAPACITY> inline_;
    OverflowProperties overflow_;
    DictionaryProperties dictionary_;
};

// =============================================================================
// Element Storage (for arrays)
// =============================================================================

enum class ElementsKind : uint8_t {
    Packed,         // Dense array, no holes
    Holey,          // Has holes (undefined elements)
    Dictionary,     // Sparse array (hash table)
    TypedArray      // Typed array backing store
};

class ElementStorage {
public:
    ElementStorage() : kind_(ElementsKind::Packed), length_(0) {}
    
    ElementsKind kind() const { return kind_; }
    size_t length() const { return length_; }
    
    Value get(size_t index) const {
        if (kind_ == ElementsKind::Dictionary) {
            return getDictionary(index);
        }
        return index < elements_.size() ? elements_[index] : Value::undefined();
    }
    
    void set(size_t index, Value value) {
        if (kind_ == ElementsKind::Dictionary) {
            setDictionary(index, value);
            return;
        }
        ensureCapacity(index + 1);
        elements_[index] = value;
        if (index >= length_) length_ = index + 1;
    }
    
    void push(Value value) {
        set(length_, value);
    }
    
    Value pop() {
        if (length_ == 0) return Value::undefined();
        Value v = get(length_ - 1);
        length_--;
        return v;
    }
    
    void setLength(size_t newLength) {
        length_ = newLength;
        if (newLength < elements_.size()) {
            elements_.resize(newLength);
        }
    }
    
private:
    ElementsKind kind_;
    size_t length_;
    std::vector<Value> elements_;
    std::unordered_map<size_t, Value> sparseElements_;
    
    void ensureCapacity(size_t needed) {
        if (needed > elements_.size()) {
            // Check if should go to dictionary mode
            if (needed > 10000 && needed > elements_.size() * 4) {
                convertToDictionary();
                return;
            }
            elements_.resize(needed, Value::undefined());
        }
    }
    
    void convertToDictionary() {
        kind_ = ElementsKind::Dictionary;
        for (size_t i = 0; i < elements_.size(); i++) {
            if (!elements_[i].isUndefined()) {
                sparseElements_[i] = elements_[i];
            }
        }
        elements_.clear();
    }
    
    Value getDictionary(size_t index) const {
        auto it = sparseElements_.find(index);
        return it != sparseElements_.end() ? it->second : Value::undefined();
    }
    
    void setDictionary(size_t index, Value value) {
        sparseElements_[index] = value;
        if (index >= length_) length_ = index + 1;
    }
};

} // namespace Zepra::Runtime
