/**
 * @file TypedArrayAPI.h
 * @brief TypedArray Implementation
 */

#pragma once

#include <vector>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <memory>
#include <stdexcept>

namespace Zepra::Runtime {

class ArrayBuffer;

template<typename T>
class TypedArray {
public:
    using value_type = T;
    
    TypedArray() = default;
    
    explicit TypedArray(size_t length) : data_(length), byteOffset_(0) {}
    
    TypedArray(std::shared_ptr<ArrayBuffer> buffer, size_t byteOffset = 0, size_t length = SIZE_MAX);
    
    TypedArray(std::initializer_list<T> init) : data_(init), byteOffset_(0) {}
    
    // Static
    static TypedArray from(const std::vector<T>& source) {
        TypedArray arr(source.size());
        std::copy(source.begin(), source.end(), arr.data_.begin());
        return arr;
    }
    
    template<typename... Args>
    static TypedArray of(Args... args) {
        return TypedArray{static_cast<T>(args)...};
    }
    
    // Properties
    size_t length() const { return data_.size(); }
    size_t byteLength() const { return data_.size() * sizeof(T); }
    size_t byteOffset() const { return byteOffset_; }
    static constexpr size_t BYTES_PER_ELEMENT = sizeof(T);
    
    // Element access
    T& operator[](size_t index) { return data_[index]; }
    const T& operator[](size_t index) const { return data_[index]; }
    
    T at(int index) const {
        if (index < 0) index += static_cast<int>(data_.size());
        if (index < 0 || index >= static_cast<int>(data_.size())) {
            throw std::out_of_range("Index out of bounds");
        }
        return data_[index];
    }
    
    // Methods
    void set(const std::vector<T>& array, size_t offset = 0) {
        for (size_t i = 0; i < array.size() && offset + i < data_.size(); ++i) {
            data_[offset + i] = array[i];
        }
    }
    
    void set(const TypedArray& array, size_t offset = 0) {
        for (size_t i = 0; i < array.length() && offset + i < data_.size(); ++i) {
            data_[offset + i] = array[i];
        }
    }
    
    TypedArray subarray(int begin = 0, int end = INT_MAX) const {
        int len = static_cast<int>(data_.size());
        if (begin < 0) begin = std::max(0, len + begin);
        if (end < 0) end = std::max(0, len + end);
        end = std::min(end, len);
        if (begin >= end) return TypedArray();
        
        TypedArray result(end - begin);
        std::copy(data_.begin() + begin, data_.begin() + end, result.data_.begin());
        return result;
    }
    
    TypedArray slice(int begin = 0, int end = INT_MAX) const {
        return subarray(begin, end);
    }
    
    void copyWithin(int target, int start, int end = INT_MAX) {
        int len = static_cast<int>(data_.size());
        if (target < 0) target = std::max(0, len + target);
        if (start < 0) start = std::max(0, len + start);
        if (end < 0) end = std::max(0, len + end);
        end = std::min(end, len);
        
        size_t count = std::min(static_cast<size_t>(end - start), data_.size() - target);
        std::memmove(&data_[target], &data_[start], count * sizeof(T));
    }
    
    void fill(T value, size_t start = 0, size_t end = SIZE_MAX) {
        end = std::min(end, data_.size());
        std::fill(data_.begin() + start, data_.begin() + end, value);
    }
    
    void reverse() { std::reverse(data_.begin(), data_.end()); }
    
    void sort() { std::sort(data_.begin(), data_.end()); }
    
    int indexOf(T value, size_t fromIndex = 0) const {
        for (size_t i = fromIndex; i < data_.size(); ++i) {
            if (data_[i] == value) return static_cast<int>(i);
        }
        return -1;
    }
    
    int lastIndexOf(T value) const {
        for (size_t i = data_.size(); i-- > 0;) {
            if (data_[i] == value) return static_cast<int>(i);
        }
        return -1;
    }
    
    bool includes(T value, size_t fromIndex = 0) const {
        return indexOf(value, fromIndex) != -1;
    }
    
    // Iterator support
    auto begin() { return data_.begin(); }
    auto end() { return data_.end(); }
    auto begin() const { return data_.begin(); }
    auto end() const { return data_.end(); }
    
    T* data() { return data_.data(); }
    const T* data() const { return data_.data(); }

private:
    std::vector<T> data_;
    size_t byteOffset_ = 0;
};

// Type aliases
using Int8Array = TypedArray<int8_t>;
using Uint8Array = TypedArray<uint8_t>;
using Int16Array = TypedArray<int16_t>;
using Uint16Array = TypedArray<uint16_t>;
using Int32Array = TypedArray<int32_t>;
using Uint32Array = TypedArray<uint32_t>;
using Float32Array = TypedArray<float>;
using Float64Array = TypedArray<double>;
using BigInt64Array = TypedArray<int64_t>;
using BigUint64Array = TypedArray<uint64_t>;

// Uint8ClampedArray
class Uint8ClampedArray : public TypedArray<uint8_t> {
public:
    using TypedArray<uint8_t>::TypedArray;
    
    void set(size_t index, int value) {
        if (index < length()) {
            (*this)[index] = static_cast<uint8_t>(std::clamp(value, 0, 255));
        }
    }
};

} // namespace Zepra::Runtime
