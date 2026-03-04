/**
 * @file RecordTupleAPI.h
 * @brief Immutable Record & Tuple (Stage 2 Proposal)
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <functional>
#include <stdexcept>

namespace Zepra::Runtime {

// =============================================================================
// Record (Immutable Object)
// =============================================================================

template<typename V>
class Record {
public:
    using Key = std::string;
    using Value = V;
    
    Record() = default;
    Record(std::initializer_list<std::pair<Key, Value>> init) 
        : data_(init.begin(), init.end()) {}
    
    const Value& get(const Key& key) const {
        auto it = data_.find(key);
        if (it == data_.end()) throw std::out_of_range("Key not found: " + key);
        return it->second;
    }
    
    std::optional<Value> tryGet(const Key& key) const {
        auto it = data_.find(key);
        return it != data_.end() ? std::optional(it->second) : std::nullopt;
    }
    
    bool has(const Key& key) const {
        return data_.find(key) != data_.end();
    }
    
    size_t size() const { return data_.size(); }
    bool empty() const { return data_.empty(); }
    
    std::vector<Key> keys() const {
        std::vector<Key> result;
        for (const auto& [k, v] : data_) result.push_back(k);
        return result;
    }
    
    std::vector<Value> values() const {
        std::vector<Value> result;
        for (const auto& [k, v] : data_) result.push_back(v);
        return result;
    }
    
    std::vector<std::pair<Key, Value>> entries() const {
        return {data_.begin(), data_.end()};
    }
    
    Record with(const Key& key, const Value& value) const {
        Record result = *this;
        result.data_[key] = value;
        return result;
    }
    
    Record without(const Key& key) const {
        Record result = *this;
        result.data_.erase(key);
        return result;
    }
    
    Record merge(const Record& other) const {
        Record result = *this;
        for (const auto& [k, v] : other.data_) {
            result.data_[k] = v;
        }
        return result;
    }
    
    bool equals(const Record& other) const {
        return data_ == other.data_;
    }
    
    bool operator==(const Record& other) const { return equals(other); }
    bool operator!=(const Record& other) const { return !equals(other); }
    
    auto begin() const { return data_.begin(); }
    auto end() const { return data_.end(); }

private:
    std::map<Key, Value> data_;
};

// =============================================================================
// Tuple (Immutable Array)
// =============================================================================

template<typename T>
class Tuple {
public:
    using Value = T;
    
    Tuple() = default;
    Tuple(std::initializer_list<T> init) : data_(init) {}
    explicit Tuple(std::vector<T> data) : data_(std::move(data)) {}
    
    const T& at(size_t index) const {
        if (index >= data_.size()) throw std::out_of_range("Index out of bounds");
        return data_[index];
    }
    
    const T& operator[](size_t index) const { return data_[index]; }
    
    size_t size() const { return data_.size(); }
    bool empty() const { return data_.empty(); }
    
    const T& first() const { return data_.front(); }
    const T& last() const { return data_.back(); }
    
    // Immutable operations
    Tuple with(size_t index, const T& value) const {
        if (index >= data_.size()) throw std::out_of_range("Index out of bounds");
        Tuple result = *this;
        result.data_[index] = value;
        return result;
    }
    
    Tuple pushed(const T& value) const {
        Tuple result = *this;
        result.data_.push_back(value);
        return result;
    }
    
    Tuple popped() const {
        if (data_.empty()) return *this;
        Tuple result;
        result.data_.assign(data_.begin(), data_.end() - 1);
        return result;
    }
    
    Tuple concat(const Tuple& other) const {
        Tuple result = *this;
        result.data_.insert(result.data_.end(), other.data_.begin(), other.data_.end());
        return result;
    }
    
    Tuple slice(size_t start, size_t end = SIZE_MAX) const {
        end = std::min(end, data_.size());
        start = std::min(start, end);
        return Tuple({data_.begin() + start, data_.begin() + end});
    }
    
    Tuple reversed() const {
        Tuple result = *this;
        std::reverse(result.data_.begin(), result.data_.end());
        return result;
    }
    
    Tuple sorted() const {
        Tuple result = *this;
        std::sort(result.data_.begin(), result.data_.end());
        return result;
    }
    
    template<typename F>
    Tuple<T> map(F&& fn) const {
        std::vector<T> result;
        for (size_t i = 0; i < data_.size(); ++i) {
            result.push_back(fn(data_[i], i));
        }
        return Tuple<T>(std::move(result));
    }
    
    template<typename F>
    Tuple filter(F&& fn) const {
        std::vector<T> result;
        for (size_t i = 0; i < data_.size(); ++i) {
            if (fn(data_[i], i)) result.push_back(data_[i]);
        }
        return Tuple(std::move(result));
    }
    
    template<typename F, typename U>
    U reduce(F&& fn, U initial) const {
        U acc = initial;
        for (size_t i = 0; i < data_.size(); ++i) {
            acc = fn(acc, data_[i], i);
        }
        return acc;
    }
    
    int indexOf(const T& value) const {
        for (size_t i = 0; i < data_.size(); ++i) {
            if (data_[i] == value) return static_cast<int>(i);
        }
        return -1;
    }
    
    bool includes(const T& value) const {
        return indexOf(value) != -1;
    }
    
    bool equals(const Tuple& other) const {
        return data_ == other.data_;
    }
    
    bool operator==(const Tuple& other) const { return equals(other); }
    bool operator!=(const Tuple& other) const { return !equals(other); }
    
    auto begin() const { return data_.begin(); }
    auto end() const { return data_.end(); }

private:
    std::vector<T> data_;
};

// =============================================================================
// Factory Functions
// =============================================================================

template<typename V, typename... Args>
Record<V> makeRecord(Args&&... args) {
    return Record<V>{std::forward<Args>(args)...};
}

template<typename T, typename... Args>
Tuple<T> makeTuple(Args&&... args) {
    return Tuple<T>{std::forward<Args>(args)...};
}

} // namespace Zepra::Runtime
