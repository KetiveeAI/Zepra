/**
 * @file StructuredCloneAPI.h
 * @brief Structured Clone Algorithm
 * 
 * HTML Standard:
 * - structuredClone(): Deep copy with transferables
 * - Serialization/Deserialization
 */

#pragma once

#include <string>
#include <vector>
#include <variant>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <stdexcept>

namespace Zepra::API {

// =============================================================================
// Cloneable Value Types
// =============================================================================

struct CloneableValue;

using CloneableArray = std::vector<std::shared_ptr<CloneableValue>>;
using CloneableObject = std::unordered_map<std::string, std::shared_ptr<CloneableValue>>;

enum class CloneableType {
    Undefined,
    Null,
    Boolean,
    Number,
    BigInt,
    String,
    Date,
    RegExp,
    Array,
    Object,
    Map,
    Set,
    ArrayBuffer,
    TypedArray,
    Blob,
    File,
    Error
};

/**
 * @brief Value that can be structured cloned
 */
struct CloneableValue {
    CloneableType type = CloneableType::Undefined;
    
    std::variant<
        std::monostate,
        bool,
        double,
        int64_t,
        std::string,
        CloneableArray,
        CloneableObject,
        std::vector<uint8_t>
    > data;
    
    // For Date
    double timestamp = 0;
    
    // For RegExp
    std::string pattern;
    std::string flags;
    
    // For Error
    std::string errorName;
    std::string errorMessage;
    
    // For TypedArray
    std::string typedArrayType;
    size_t byteOffset = 0;
    size_t byteLength = 0;
};

// =============================================================================
// Clone Context
// =============================================================================

/**
 * @brief Tracks objects during cloning to handle cycles
 */
class CloneContext {
public:
    // Check if already cloned
    bool hasCloned(const void* original) const {
        return clonedObjects_.count(original) > 0;
    }
    
    // Get cloned reference
    std::shared_ptr<CloneableValue> getCloned(const void* original) const {
        auto it = clonedObjects_.find(original);
        return it != clonedObjects_.end() ? it->second : nullptr;
    }
    
    // Register cloned object
    void registerClone(const void* original, std::shared_ptr<CloneableValue> clone) {
        clonedObjects_[original] = clone;
    }
    
    // Transferables
    void addTransferable(std::shared_ptr<std::vector<uint8_t>> buffer) {
        transferables_.insert(buffer);
    }
    
    bool isTransferable(const std::shared_ptr<std::vector<uint8_t>>& buffer) const {
        return transferables_.count(buffer) > 0;
    }
    
private:
    std::unordered_map<const void*, std::shared_ptr<CloneableValue>> clonedObjects_;
    std::unordered_set<std::shared_ptr<std::vector<uint8_t>>> transferables_;
};

// =============================================================================
// Structured Clone
// =============================================================================

/**
 * @brief Implements the structured clone algorithm
 */
class StructuredClone {
public:
    using TransferList = std::vector<std::shared_ptr<std::vector<uint8_t>>>;
    
    // Clone a value
    static std::shared_ptr<CloneableValue> clone(
            const CloneableValue& value,
            const TransferList& transfer = {}) {
        
        CloneContext ctx;
        
        // Mark transferables
        for (const auto& t : transfer) {
            ctx.addTransferable(t);
        }
        
        return cloneInternal(value, ctx);
    }
    
private:
    static std::shared_ptr<CloneableValue> cloneInternal(
            const CloneableValue& value,
            CloneContext& ctx) {
        
        auto result = std::make_shared<CloneableValue>();
        result->type = value.type;
        
        switch (value.type) {
            case CloneableType::Undefined:
            case CloneableType::Null:
                break;
                
            case CloneableType::Boolean:
            case CloneableType::Number:
            case CloneableType::BigInt:
            case CloneableType::String:
                result->data = value.data;
                break;
                
            case CloneableType::Date:
                result->timestamp = value.timestamp;
                break;
                
            case CloneableType::RegExp:
                result->pattern = value.pattern;
                result->flags = value.flags;
                break;
                
            case CloneableType::Array: {
                const auto& arr = std::get<CloneableArray>(value.data);
                CloneableArray cloned;
                cloned.reserve(arr.size());
                
                for (const auto& item : arr) {
                    cloned.push_back(cloneInternal(*item, ctx));
                }
                
                result->data = std::move(cloned);
                break;
            }
                
            case CloneableType::Object: {
                const auto& obj = std::get<CloneableObject>(value.data);
                CloneableObject cloned;
                
                for (const auto& [key, val] : obj) {
                    cloned[key] = cloneInternal(*val, ctx);
                }
                
                result->data = std::move(cloned);
                break;
            }
                
            case CloneableType::ArrayBuffer: {
                const auto& buffer = std::get<std::vector<uint8_t>>(value.data);
                result->data = buffer;  // Copy
                break;
            }
                
            case CloneableType::TypedArray:
                result->typedArrayType = value.typedArrayType;
                result->byteOffset = value.byteOffset;
                result->byteLength = value.byteLength;
                result->data = std::get<std::vector<uint8_t>>(value.data);
                break;
                
            case CloneableType::Error:
                result->errorName = value.errorName;
                result->errorMessage = value.errorMessage;
                break;
                
            default:
                throw std::runtime_error("Cannot clone this type");
        }
        
        return result;
    }
};

// =============================================================================
// Serialization (for postMessage, IndexedDB, etc.)
// =============================================================================

/**
 * @brief Serialize to binary format
 */
class StructuredSerializer {
public:
    static std::vector<uint8_t> serialize(const CloneableValue& value) {
        std::vector<uint8_t> buffer;
        serializeInternal(value, buffer);
        return buffer;
    }
    
    static CloneableValue deserialize(const std::vector<uint8_t>& buffer) {
        size_t offset = 0;
        return deserializeInternal(buffer, offset);
    }
    
private:
    static void serializeInternal(const CloneableValue& value,
                                   std::vector<uint8_t>& buffer) {
        // Write type byte
        buffer.push_back(static_cast<uint8_t>(value.type));
        
        switch (value.type) {
            case CloneableType::Undefined:
            case CloneableType::Null:
                break;
                
            case CloneableType::Boolean:
                buffer.push_back(std::get<bool>(value.data) ? 1 : 0);
                break;
                
            case CloneableType::Number: {
                double num = std::get<double>(value.data);
                auto* bytes = reinterpret_cast<uint8_t*>(&num);
                buffer.insert(buffer.end(), bytes, bytes + 8);
                break;
            }
                
            case CloneableType::String: {
                const auto& str = std::get<std::string>(value.data);
                uint32_t len = str.length();
                auto* lenBytes = reinterpret_cast<uint8_t*>(&len);
                buffer.insert(buffer.end(), lenBytes, lenBytes + 4);
                buffer.insert(buffer.end(), str.begin(), str.end());
                break;
            }
                
            case CloneableType::Array: {
                const auto& arr = std::get<CloneableArray>(value.data);
                uint32_t len = arr.size();
                auto* lenBytes = reinterpret_cast<uint8_t*>(&len);
                buffer.insert(buffer.end(), lenBytes, lenBytes + 4);
                for (const auto& item : arr) {
                    serializeInternal(*item, buffer);
                }
                break;
            }
                
            case CloneableType::Object: {
                const auto& obj = std::get<CloneableObject>(value.data);
                uint32_t len = obj.size();
                auto* lenBytes = reinterpret_cast<uint8_t*>(&len);
                buffer.insert(buffer.end(), lenBytes, lenBytes + 4);
                for (const auto& [key, val] : obj) {
                    uint32_t keyLen = key.length();
                    auto* keyLenBytes = reinterpret_cast<uint8_t*>(&keyLen);
                    buffer.insert(buffer.end(), keyLenBytes, keyLenBytes + 4);
                    buffer.insert(buffer.end(), key.begin(), key.end());
                    serializeInternal(*val, buffer);
                }
                break;
            }
                
            case CloneableType::ArrayBuffer: {
                const auto& data = std::get<std::vector<uint8_t>>(value.data);
                uint32_t len = data.size();
                auto* lenBytes = reinterpret_cast<uint8_t*>(&len);
                buffer.insert(buffer.end(), lenBytes, lenBytes + 4);
                buffer.insert(buffer.end(), data.begin(), data.end());
                break;
            }
                
            default:
                break;
        }
    }
    
    static CloneableValue deserializeInternal(const std::vector<uint8_t>& buffer,
                                               size_t& offset) {
        CloneableValue result;
        
        if (offset >= buffer.size()) {
            result.type = CloneableType::Undefined;
            return result;
        }
        
        result.type = static_cast<CloneableType>(buffer[offset++]);
        
        switch (result.type) {
            case CloneableType::Boolean:
                result.data = buffer[offset++] != 0;
                break;
                
            case CloneableType::Number: {
                double num;
                std::memcpy(&num, &buffer[offset], 8);
                offset += 8;
                result.data = num;
                break;
            }
                
            case CloneableType::String: {
                uint32_t len;
                std::memcpy(&len, &buffer[offset], 4);
                offset += 4;
                result.data = std::string(
                    reinterpret_cast<const char*>(&buffer[offset]), len);
                offset += len;
                break;
            }
                
            case CloneableType::Array: {
                uint32_t len;
                std::memcpy(&len, &buffer[offset], 4);
                offset += 4;
                CloneableArray arr;
                for (uint32_t i = 0; i < len; i++) {
                    arr.push_back(std::make_shared<CloneableValue>(
                        deserializeInternal(buffer, offset)));
                }
                result.data = std::move(arr);
                break;
            }
                
            case CloneableType::Object: {
                uint32_t len;
                std::memcpy(&len, &buffer[offset], 4);
                offset += 4;
                CloneableObject obj;
                for (uint32_t i = 0; i < len; i++) {
                    uint32_t keyLen;
                    std::memcpy(&keyLen, &buffer[offset], 4);
                    offset += 4;
                    std::string key(
                        reinterpret_cast<const char*>(&buffer[offset]), keyLen);
                    offset += keyLen;
                    obj[key] = std::make_shared<CloneableValue>(
                        deserializeInternal(buffer, offset));
                }
                result.data = std::move(obj);
                break;
            }
                
            case CloneableType::ArrayBuffer: {
                uint32_t len;
                std::memcpy(&len, &buffer[offset], 4);
                offset += 4;
                result.data = std::vector<uint8_t>(
                    buffer.begin() + offset,
                    buffer.begin() + offset + len);
                offset += len;
                break;
            }
                
            default:
                break;
        }
        
        return result;
    }
};

// =============================================================================
// Global structuredClone
// =============================================================================

template<typename T>
T structuredClone(const T& value);

} // namespace Zepra::API
