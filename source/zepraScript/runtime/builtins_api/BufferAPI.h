/**
 * @file BufferAPI.h
 * @brief ArrayBuffer and DataView Implementation
 */

#pragma once

#include <vector>
#include <cstdint>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <algorithm>

namespace Zepra::Runtime {

// =============================================================================
// ArrayBuffer
// =============================================================================

class ArrayBuffer : public std::enable_shared_from_this<ArrayBuffer> {
public:
    ArrayBuffer() = default;
    explicit ArrayBuffer(size_t byteLength) : data_(byteLength, 0) {}
    
    // Properties
    size_t byteLength() const { return data_.size(); }
    bool detached() const { return detached_; }
    bool resizable() const { return maxByteLength_ > 0; }
    size_t maxByteLength() const { return maxByteLength_; }
    
    // Static
    static bool isView(const void*) { return false; }
    
    // Methods
    std::shared_ptr<ArrayBuffer> slice(size_t begin = 0, size_t end = SIZE_MAX) const {
        if (detached_) throw std::runtime_error("ArrayBuffer is detached");
        end = std::min(end, data_.size());
        begin = std::min(begin, end);
        
        auto result = std::make_shared<ArrayBuffer>(end - begin);
        std::copy(data_.begin() + begin, data_.begin() + end, result->data_.begin());
        return result;
    }
    
    std::shared_ptr<ArrayBuffer> transfer(size_t newByteLength = SIZE_MAX) {
        if (detached_) throw std::runtime_error("ArrayBuffer is detached");
        if (newByteLength == SIZE_MAX) newByteLength = data_.size();
        
        auto result = std::make_shared<ArrayBuffer>(newByteLength);
        size_t copyLen = std::min(data_.size(), newByteLength);
        std::copy(data_.begin(), data_.begin() + copyLen, result->data_.begin());
        
        detached_ = true;
        data_.clear();
        return result;
    }
    
    void resize(size_t newByteLength) {
        if (!resizable()) throw std::runtime_error("ArrayBuffer is not resizable");
        if (newByteLength > maxByteLength_) throw std::runtime_error("Exceeds maxByteLength");
        data_.resize(newByteLength);
    }
    
    // Data access
    uint8_t* data() { return data_.data(); }
    const uint8_t* data() const { return data_.data(); }

private:
    std::vector<uint8_t> data_;
    bool detached_ = false;
    size_t maxByteLength_ = 0;
};

// =============================================================================
// SharedArrayBuffer
// =============================================================================

class SharedArrayBuffer : public std::enable_shared_from_this<SharedArrayBuffer> {
public:
    SharedArrayBuffer() = default;
    explicit SharedArrayBuffer(size_t byteLength) : data_(std::make_shared<std::vector<uint8_t>>(byteLength, 0)) {}
    
    size_t byteLength() const { return data_->size(); }
    bool growable() const { return maxByteLength_ > 0; }
    size_t maxByteLength() const { return maxByteLength_; }
    
    std::shared_ptr<SharedArrayBuffer> slice(size_t begin = 0, size_t end = SIZE_MAX) const {
        end = std::min(end, data_->size());
        begin = std::min(begin, end);
        
        auto result = std::make_shared<SharedArrayBuffer>(end - begin);
        std::copy(data_->begin() + begin, data_->begin() + end, result->data_->begin());
        return result;
    }
    
    void grow(size_t newByteLength) {
        if (!growable()) throw std::runtime_error("SharedArrayBuffer is not growable");
        if (newByteLength > maxByteLength_) throw std::runtime_error("Exceeds maxByteLength");
        data_->resize(newByteLength);
    }
    
    uint8_t* data() { return data_->data(); }
    const uint8_t* data() const { return data_->data(); }

private:
    std::shared_ptr<std::vector<uint8_t>> data_;
    size_t maxByteLength_ = 0;
};

// =============================================================================
// DataView
// =============================================================================

class DataView {
public:
    DataView(std::shared_ptr<ArrayBuffer> buffer, size_t byteOffset = 0, size_t byteLength = SIZE_MAX)
        : buffer_(buffer)
        , byteOffset_(byteOffset)
        , byteLength_(byteLength == SIZE_MAX ? buffer->byteLength() - byteOffset : byteLength) {}
    
    // Properties
    std::shared_ptr<ArrayBuffer> buffer() const { return buffer_; }
    size_t byteOffset() const { return byteOffset_; }
    size_t byteLength() const { return byteLength_; }
    
    // Getters
    int8_t getInt8(size_t offset) const { return get<int8_t>(offset); }
    uint8_t getUint8(size_t offset) const { return get<uint8_t>(offset); }
    int16_t getInt16(size_t offset, bool littleEndian = false) const { return getMulti<int16_t>(offset, littleEndian); }
    uint16_t getUint16(size_t offset, bool littleEndian = false) const { return getMulti<uint16_t>(offset, littleEndian); }
    int32_t getInt32(size_t offset, bool littleEndian = false) const { return getMulti<int32_t>(offset, littleEndian); }
    uint32_t getUint32(size_t offset, bool littleEndian = false) const { return getMulti<uint32_t>(offset, littleEndian); }
    float getFloat32(size_t offset, bool littleEndian = false) const { return getMulti<float>(offset, littleEndian); }
    double getFloat64(size_t offset, bool littleEndian = false) const { return getMulti<double>(offset, littleEndian); }
    int64_t getBigInt64(size_t offset, bool littleEndian = false) const { return getMulti<int64_t>(offset, littleEndian); }
    uint64_t getBigUint64(size_t offset, bool littleEndian = false) const { return getMulti<uint64_t>(offset, littleEndian); }
    
    // Setters
    void setInt8(size_t offset, int8_t value) { set<int8_t>(offset, value); }
    void setUint8(size_t offset, uint8_t value) { set<uint8_t>(offset, value); }
    void setInt16(size_t offset, int16_t value, bool littleEndian = false) { setMulti<int16_t>(offset, value, littleEndian); }
    void setUint16(size_t offset, uint16_t value, bool littleEndian = false) { setMulti<uint16_t>(offset, value, littleEndian); }
    void setInt32(size_t offset, int32_t value, bool littleEndian = false) { setMulti<int32_t>(offset, value, littleEndian); }
    void setUint32(size_t offset, uint32_t value, bool littleEndian = false) { setMulti<uint32_t>(offset, value, littleEndian); }
    void setFloat32(size_t offset, float value, bool littleEndian = false) { setMulti<float>(offset, value, littleEndian); }
    void setFloat64(size_t offset, double value, bool littleEndian = false) { setMulti<double>(offset, value, littleEndian); }
    void setBigInt64(size_t offset, int64_t value, bool littleEndian = false) { setMulti<int64_t>(offset, value, littleEndian); }
    void setBigUint64(size_t offset, uint64_t value, bool littleEndian = false) { setMulti<uint64_t>(offset, value, littleEndian); }

private:
    template<typename T>
    T get(size_t offset) const {
        checkBounds(offset, sizeof(T));
        return *reinterpret_cast<const T*>(buffer_->data() + byteOffset_ + offset);
    }
    
    template<typename T>
    void set(size_t offset, T value) {
        checkBounds(offset, sizeof(T));
        *reinterpret_cast<T*>(buffer_->data() + byteOffset_ + offset) = value;
    }
    
    template<typename T>
    T getMulti(size_t offset, bool littleEndian) const {
        checkBounds(offset, sizeof(T));
        T value;
        std::memcpy(&value, buffer_->data() + byteOffset_ + offset, sizeof(T));
        if (needsSwap(littleEndian)) value = byteSwap(value);
        return value;
    }
    
    template<typename T>
    void setMulti(size_t offset, T value, bool littleEndian) {
        checkBounds(offset, sizeof(T));
        if (needsSwap(littleEndian)) value = byteSwap(value);
        std::memcpy(buffer_->data() + byteOffset_ + offset, &value, sizeof(T));
    }
    
    void checkBounds(size_t offset, size_t size) const {
        if (offset + size > byteLength_) throw std::out_of_range("DataView access out of bounds");
    }
    
    static bool needsSwap(bool littleEndian) {
        uint16_t test = 1;
        bool isLittleEndian = *reinterpret_cast<uint8_t*>(&test) == 1;
        return isLittleEndian != littleEndian;
    }
    
    template<typename T>
    static T byteSwap(T value) {
        T result;
        auto* src = reinterpret_cast<uint8_t*>(&value);
        auto* dst = reinterpret_cast<uint8_t*>(&result);
        for (size_t i = 0; i < sizeof(T); ++i) {
            dst[i] = src[sizeof(T) - 1 - i];
        }
        return result;
    }
    
    std::shared_ptr<ArrayBuffer> buffer_;
    size_t byteOffset_;
    size_t byteLength_;
};

} // namespace Zepra::Runtime
