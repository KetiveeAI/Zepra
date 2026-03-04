/**
 * @file ArrayAPI.h
 * @brief Array Implementation with ES2024 methods
 */

#pragma once

#include <vector>
#include <functional>
#include <optional>
#include <algorithm>
#include <numeric>
#include <variant>
#include <memory>

namespace Zepra::Runtime {

template<typename T>
class Array {
public:
    using Value = T;
    using Predicate = std::function<bool(const T&, size_t, const Array&)>;
    using Mapper = std::function<T(const T&, size_t, const Array&)>;
    using Reducer = std::function<T(const T&, const T&, size_t, const Array&)>;
    using Callback = std::function<void(const T&, size_t, const Array&)>;
    
    Array() = default;
    Array(std::initializer_list<T> init) : data_(init) {}
    explicit Array(size_t size) : data_(size) {}
    explicit Array(std::vector<T> data) : data_(std::move(data)) {}
    
    // Static methods
    static Array from(const std::vector<T>& iterable) { return Array(iterable); }
    
    template<typename... Args>
    static Array of(Args&&... args) { return Array{std::forward<Args>(args)...}; }
    
    static bool isArray(const Array&) { return true; }
    
    // Properties
    size_t length() const { return data_.size(); }
    bool empty() const { return data_.empty(); }
    
    // Element access
    T& operator[](size_t index) { return data_[index]; }
    const T& operator[](size_t index) const { return data_[index]; }
    
    std::optional<T> at(int index) const {
        if (index < 0) index += static_cast<int>(data_.size());
        if (index < 0 || index >= static_cast<int>(data_.size())) return std::nullopt;
        return data_[index];
    }
    
    // Mutating methods
    void push(const T& value) { data_.push_back(value); }
    void push(T&& value) { data_.push_back(std::move(value)); }
    
    std::optional<T> pop() {
        if (data_.empty()) return std::nullopt;
        T value = std::move(data_.back());
        data_.pop_back();
        return value;
    }
    
    std::optional<T> shift() {
        if (data_.empty()) return std::nullopt;
        T value = std::move(data_.front());
        data_.erase(data_.begin());
        return value;
    }
    
    void unshift(const T& value) { data_.insert(data_.begin(), value); }
    
    void reverse() { std::reverse(data_.begin(), data_.end()); }
    
    void sort() { std::sort(data_.begin(), data_.end()); }
    
    void sort(std::function<int(const T&, const T&)> compareFn) {
        std::sort(data_.begin(), data_.end(), [&](const T& a, const T& b) {
            return compareFn(a, b) < 0;
        });
    }
    
    Array splice(size_t start, size_t deleteCount, const std::vector<T>& items = {}) {
        start = std::min(start, data_.size());
        deleteCount = std::min(deleteCount, data_.size() - start);
        
        Array removed;
        for (size_t i = 0; i < deleteCount; ++i) {
            removed.push(data_[start + i]);
        }
        
        data_.erase(data_.begin() + start, data_.begin() + start + deleteCount);
        data_.insert(data_.begin() + start, items.begin(), items.end());
        
        return removed;
    }
    
    void fill(const T& value, size_t start = 0, size_t end = SIZE_MAX) {
        end = std::min(end, data_.size());
        for (size_t i = start; i < end; ++i) data_[i] = value;
    }
    
    void copyWithin(size_t target, size_t start, size_t end = SIZE_MAX) {
        end = std::min(end, data_.size());
        size_t len = std::min(end - start, data_.size() - target);
        std::vector<T> temp(data_.begin() + start, data_.begin() + start + len);
        std::copy(temp.begin(), temp.end(), data_.begin() + target);
    }
    
    // Non-mutating methods (ES2023+ immutable versions)
    Array toReversed() const {
        Array result = *this;
        result.reverse();
        return result;
    }
    
    Array toSorted() const {
        Array result = *this;
        result.sort();
        return result;
    }
    
    Array toSorted(std::function<int(const T&, const T&)> compareFn) const {
        Array result = *this;
        result.sort(compareFn);
        return result;
    }
    
    Array toSpliced(size_t start, size_t deleteCount, const std::vector<T>& items = {}) const {
        Array result = *this;
        result.splice(start, deleteCount, items);
        return result;
    }
    
    Array with(size_t index, const T& value) const {
        Array result = *this;
        if (index < result.data_.size()) {
            result.data_[index] = value;
        }
        return result;
    }
    
    // Iteration methods
    void forEach(Callback fn) const {
        for (size_t i = 0; i < data_.size(); ++i) fn(data_[i], i, *this);
    }
    
    Array map(Mapper fn) const {
        Array result;
        for (size_t i = 0; i < data_.size(); ++i) {
            result.push(fn(data_[i], i, *this));
        }
        return result;
    }
    
    Array filter(Predicate fn) const {
        Array result;
        for (size_t i = 0; i < data_.size(); ++i) {
            if (fn(data_[i], i, *this)) result.push(data_[i]);
        }
        return result;
    }
    
    T reduce(Reducer fn, T initial) const {
        T acc = initial;
        for (size_t i = 0; i < data_.size(); ++i) {
            acc = fn(acc, data_[i], i, *this);
        }
        return acc;
    }
    
    T reduceRight(Reducer fn, T initial) const {
        T acc = initial;
        for (size_t i = data_.size(); i-- > 0;) {
            acc = fn(acc, data_[i], i, *this);
        }
        return acc;
    }
    
    // Search methods
    std::optional<T> find(Predicate fn) const {
        for (size_t i = 0; i < data_.size(); ++i) {
            if (fn(data_[i], i, *this)) return data_[i];
        }
        return std::nullopt;
    }
    
    std::optional<T> findLast(Predicate fn) const {
        for (size_t i = data_.size(); i-- > 0;) {
            if (fn(data_[i], i, *this)) return data_[i];
        }
        return std::nullopt;
    }
    
    int findIndex(Predicate fn) const {
        for (size_t i = 0; i < data_.size(); ++i) {
            if (fn(data_[i], i, *this)) return static_cast<int>(i);
        }
        return -1;
    }
    
    int findLastIndex(Predicate fn) const {
        for (size_t i = data_.size(); i-- > 0;) {
            if (fn(data_[i], i, *this)) return static_cast<int>(i);
        }
        return -1;
    }
    
    int indexOf(const T& value, size_t fromIndex = 0) const {
        for (size_t i = fromIndex; i < data_.size(); ++i) {
            if (data_[i] == value) return static_cast<int>(i);
        }
        return -1;
    }
    
    int lastIndexOf(const T& value) const {
        for (size_t i = data_.size(); i-- > 0;) {
            if (data_[i] == value) return static_cast<int>(i);
        }
        return -1;
    }
    
    bool includes(const T& value, size_t fromIndex = 0) const {
        return indexOf(value, fromIndex) != -1;
    }
    
    // Testing methods
    bool every(Predicate fn) const {
        for (size_t i = 0; i < data_.size(); ++i) {
            if (!fn(data_[i], i, *this)) return false;
        }
        return true;
    }
    
    bool some(Predicate fn) const {
        for (size_t i = 0; i < data_.size(); ++i) {
            if (fn(data_[i], i, *this)) return true;
        }
        return false;
    }
    
    // Slice/concat
    Array slice(int start = 0, int end = INT_MAX) const {
        int len = static_cast<int>(data_.size());
        if (start < 0) start = std::max(0, len + start);
        if (end < 0) end = std::max(0, len + end);
        end = std::min(end, len);
        
        Array result;
        for (int i = start; i < end; ++i) result.push(data_[i]);
        return result;
    }
    
    Array concat(const Array& other) const {
        Array result = *this;
        for (const auto& item : other.data_) result.push(item);
        return result;
    }
    
    // Flatten
    template<typename U = T>
    Array<U> flat(int depth = 1) const {
        Array<U> result;
        flattenInto(result, depth);
        return result;
    }
    
    template<typename U>
    Array<U> flatMap(std::function<Array<U>(const T&, size_t, const Array&)> fn) const {
        Array<U> result;
        for (size_t i = 0; i < data_.size(); ++i) {
            auto mapped = fn(data_[i], i, *this);
            for (const auto& item : mapped.data_) result.push(item);
        }
        return result;
    }
    
    // Join
    std::string join(const std::string& separator = ",") const {
        std::string result;
        for (size_t i = 0; i < data_.size(); ++i) {
            if (i > 0) result += separator;
            if constexpr (std::is_same_v<T, std::string>) {
                result += data_[i];
            }
        }
        return result;
    }
    
    // Iterator support
    auto begin() { return data_.begin(); }
    auto end() { return data_.end(); }
    auto begin() const { return data_.begin(); }
    auto end() const { return data_.end(); }
    
    const std::vector<T>& data() const { return data_; }

private:
    template<typename U>
    void flattenInto(Array<U>& result, int depth) const {
        for (const auto& item : data_) {
            if constexpr (std::is_same_v<T, U>) {
                result.push(item);
            }
        }
    }
    
    std::vector<T> data_;
};

} // namespace Zepra::Runtime
