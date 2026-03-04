/**
 * @file DataViewExtendedAPI.h
 * @brief DataView Extensions (BigInt64, Float16)
 */

#pragma once

#include <cstdint>
#include <cstring>
#include <memory>
#include <stdexcept>

namespace Zepra::Runtime {

// =============================================================================
// Float16 Support
// =============================================================================

class Float16 {
public:
    Float16() : bits_(0) {}
    explicit Float16(float value) : bits_(floatToHalf(value)) {}
    
    operator float() const { return halfToFloat(bits_); }
    
    uint16_t bits() const { return bits_; }
    static Float16 fromBits(uint16_t bits) {
        Float16 f;
        f.bits_ = bits;
        return f;
    }

private:
    static uint16_t floatToHalf(float value) {
        uint32_t f;
        std::memcpy(&f, &value, sizeof(float));
        
        uint32_t sign = (f >> 16) & 0x8000;
        int32_t exp = ((f >> 23) & 0xFF) - 127 + 15;
        uint32_t mantissa = f & 0x7FFFFF;
        
        if (exp <= 0) {
            if (exp < -10) return sign;
            mantissa = (mantissa | 0x800000) >> (1 - exp);
            return sign | (mantissa >> 13);
        }
        
        if (exp >= 31) return sign | 0x7C00;
        
        return sign | (exp << 10) | (mantissa >> 13);
    }
    
    static float halfToFloat(uint16_t h) {
        uint32_t sign = (h & 0x8000) << 16;
        uint32_t exp = (h >> 10) & 0x1F;
        uint32_t mantissa = h & 0x3FF;
        
        if (exp == 0) {
            if (mantissa == 0) {
                uint32_t f = sign;
                float result;
                std::memcpy(&result, &f, sizeof(float));
                return result;
            }
            while (!(mantissa & 0x400)) {
                mantissa <<= 1;
                exp--;
            }
            exp++;
            mantissa &= ~0x400;
        } else if (exp == 31) {
            uint32_t f = sign | 0x7F800000 | (mantissa << 13);
            float result;
            std::memcpy(&result, &f, sizeof(float));
            return result;
        }
        
        exp = exp + (127 - 15);
        uint32_t f = sign | (exp << 23) | (mantissa << 13);
        float result;
        std::memcpy(&result, &f, sizeof(float));
        return result;
    }
    
    uint16_t bits_;
};

// =============================================================================
// Extended DataView
// =============================================================================

class DataViewExtended {
public:
    DataViewExtended(uint8_t* buffer, size_t byteLength, size_t byteOffset = 0)
        : buffer_(buffer + byteOffset), byteLength_(byteLength - byteOffset) {}
    
    // BigInt64
    int64_t getBigInt64(size_t byteOffset, bool littleEndian = false) const {
        checkBounds(byteOffset, 8);
        return readValue<int64_t>(byteOffset, littleEndian);
    }
    
    void setBigInt64(size_t byteOffset, int64_t value, bool littleEndian = false) {
        checkBounds(byteOffset, 8);
        writeValue(byteOffset, value, littleEndian);
    }
    
    // BigUint64
    uint64_t getBigUint64(size_t byteOffset, bool littleEndian = false) const {
        checkBounds(byteOffset, 8);
        return readValue<uint64_t>(byteOffset, littleEndian);
    }
    
    void setBigUint64(size_t byteOffset, uint64_t value, bool littleEndian = false) {
        checkBounds(byteOffset, 8);
        writeValue(byteOffset, value, littleEndian);
    }
    
    // Float16
    float getFloat16(size_t byteOffset, bool littleEndian = false) const {
        checkBounds(byteOffset, 2);
        uint16_t bits = readValue<uint16_t>(byteOffset, littleEndian);
        return Float16::fromBits(bits);
    }
    
    void setFloat16(size_t byteOffset, float value, bool littleEndian = false) {
        checkBounds(byteOffset, 2);
        Float16 f(value);
        writeValue(byteOffset, f.bits(), littleEndian);
    }
    
    size_t byteLength() const { return byteLength_; }

private:
    void checkBounds(size_t offset, size_t size) const {
        if (offset + size > byteLength_) {
            throw std::range_error("DataView offset out of bounds");
        }
    }
    
    template<typename T>
    T readValue(size_t offset, bool littleEndian) const {
        T value;
        std::memcpy(&value, buffer_ + offset, sizeof(T));
        
        if (littleEndian != isLittleEndian()) {
            value = swapBytes(value);
        }
        
        return value;
    }
    
    template<typename T>
    void writeValue(size_t offset, T value, bool littleEndian) {
        if (littleEndian != isLittleEndian()) {
            value = swapBytes(value);
        }
        std::memcpy(buffer_ + offset, &value, sizeof(T));
    }
    
    static bool isLittleEndian() {
        uint16_t test = 1;
        return *reinterpret_cast<uint8_t*>(&test) == 1;
    }
    
    template<typename T>
    static T swapBytes(T value) {
        T result;
        auto* src = reinterpret_cast<uint8_t*>(&value);
        auto* dst = reinterpret_cast<uint8_t*>(&result);
        for (size_t i = 0; i < sizeof(T); ++i) {
            dst[i] = src[sizeof(T) - 1 - i];
        }
        return result;
    }
    
    uint8_t* buffer_;
    size_t byteLength_;
};

} // namespace Zepra::Runtime
