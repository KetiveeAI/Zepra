/**
 * @file MapSetAPI.h
 * @brief Map and Set Implementation
 */

#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <functional>
#include <optional>
#include <memory>

namespace Zepra::Runtime {

// =============================================================================
// Map
// =============================================================================

template<typename K, typename V>
class Map {
public:
    using Entry = std::pair<K, V>;
    using Callback = std::function<void(const V&, const K&, const Map&)>;
    
    Map() = default;
    Map(std::initializer_list<Entry> init) {
        for (const auto& [k, v] : init) set(k, v);
    }
    
    // Properties
    size_t size() const { return order_.size(); }
    bool empty() const { return order_.empty(); }
    
    // Methods
    Map& set(const K& key, const V& value) {
        auto it = data_.find(key);
        if (it == data_.end()) {
            order_.push_back(key);
        }
        data_[key] = value;
        return *this;
    }
    
    std::optional<V> get(const K& key) const {
        auto it = data_.find(key);
        return it != data_.end() ? std::optional<V>(it->second) : std::nullopt;
    }
    
    bool has(const K& key) const {
        return data_.find(key) != data_.end();
    }
    
    bool delete_(const K& key) {
        auto it = data_.find(key);
        if (it == data_.end()) return false;
        data_.erase(it);
        order_.erase(std::remove(order_.begin(), order_.end(), key), order_.end());
        return true;
    }
    
    void clear() {
        data_.clear();
        order_.clear();
    }
    
    void forEach(Callback fn) const {
        for (const auto& key : order_) {
            fn(data_.at(key), key, *this);
        }
    }
    
    // Iterators
    std::vector<K> keys() const { return order_; }
    
    std::vector<V> values() const {
        std::vector<V> result;
        for (const auto& key : order_) {
            result.push_back(data_.at(key));
        }
        return result;
    }
    
    std::vector<Entry> entries() const {
        std::vector<Entry> result;
        for (const auto& key : order_) {
            result.emplace_back(key, data_.at(key));
        }
        return result;
    }
    
    auto begin() const { return entries().begin(); }
    auto end() const { return entries().end(); }

private:
    std::unordered_map<K, V> data_;
    std::vector<K> order_;
};

// =============================================================================
// Set
// =============================================================================

template<typename T>
class Set {
public:
    using Callback = std::function<void(const T&, const T&, const Set&)>;
    
    Set() = default;
    Set(std::initializer_list<T> init) {
        for (const auto& v : init) add(v);
    }
    
    // Properties
    size_t size() const { return order_.size(); }
    bool empty() const { return order_.empty(); }
    
    // Methods
    Set& add(const T& value) {
        if (data_.find(value) == data_.end()) {
            data_.insert(value);
            order_.push_back(value);
        }
        return *this;
    }
    
    bool has(const T& value) const {
        return data_.find(value) != data_.end();
    }
    
    bool delete_(const T& value) {
        auto it = data_.find(value);
        if (it == data_.end()) return false;
        data_.erase(it);
        order_.erase(std::remove(order_.begin(), order_.end(), value), order_.end());
        return true;
    }
    
    void clear() {
        data_.clear();
        order_.clear();
    }
    
    void forEach(Callback fn) const {
        for (const auto& value : order_) {
            fn(value, value, *this);
        }
    }
    
    // Iterators
    std::vector<T> keys() const { return order_; }
    std::vector<T> values() const { return order_; }
    
    std::vector<std::pair<T, T>> entries() const {
        std::vector<std::pair<T, T>> result;
        for (const auto& value : order_) {
            result.emplace_back(value, value);
        }
        return result;
    }
    
    auto begin() const { return order_.begin(); }
    auto end() const { return order_.end(); }
    
    // Set operations
    Set union_(const Set& other) const {
        Set result = *this;
        for (const auto& v : other.order_) result.add(v);
        return result;
    }
    
    Set intersection(const Set& other) const {
        Set result;
        for (const auto& v : order_) {
            if (other.has(v)) result.add(v);
        }
        return result;
    }
    
    Set difference(const Set& other) const {
        Set result;
        for (const auto& v : order_) {
            if (!other.has(v)) result.add(v);
        }
        return result;
    }
    
    Set symmetricDifference(const Set& other) const {
        Set result;
        for (const auto& v : order_) {
            if (!other.has(v)) result.add(v);
        }
        for (const auto& v : other.order_) {
            if (!has(v)) result.add(v);
        }
        return result;
    }
    
    bool isSubsetOf(const Set& other) const {
        for (const auto& v : order_) {
            if (!other.has(v)) return false;
        }
        return true;
    }
    
    bool isSupersetOf(const Set& other) const {
        return other.isSubsetOf(*this);
    }
    
    bool isDisjointFrom(const Set& other) const {
        for (const auto& v : order_) {
            if (other.has(v)) return false;
        }
        return true;
    }

private:
    std::unordered_set<T> data_;
    std::vector<T> order_;
};

} // namespace Zepra::Runtime
