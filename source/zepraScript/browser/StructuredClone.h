/**
 * @file StructuredClone.h
 * @brief Structured Clone Algorithm for deep copying JS values
 * 
 * Used by:
 * - Worker.postMessage() for message serialization
 * - IndexedDB for storing/retrieving values
 * - history.pushState/replaceState
 * 
 * Reference: https://html.spec.whatwg.org/multipage/structured-data.html
 */

#pragma once

#include "../config.hpp"
#include "runtime/objects/value.hpp"
#include "runtime/objects/object.hpp"
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <stdexcept>

namespace Zepra::Browser {

using Runtime::Value;
using Runtime::Object;

/**
 * @brief Error thrown when cloning fails
 */
class DataCloneError : public std::runtime_error {
public:
    explicit DataCloneError(const std::string& msg)
        : std::runtime_error("DataCloneError: " + msg) {}
};

/**
 * @brief Serialized data format
 */
struct SerializedData {
    std::vector<uint8_t> buffer;
    std::vector<Object*> transferables;
};

/**
 * @brief Structured Clone implementation
 */
class StructuredClone {
public:
    /**
     * @brief Clone a value (structuredClone global function)
     * @param input Value to clone
     * @param transferList Objects to transfer (not copy)
     * @return Deep cloned value
     * @throws DataCloneError if value cannot be cloned
     */
    static Value clone(const Value& input, 
                       const std::vector<Object*>& transferList = {});
    
    /**
     * @brief Serialize value to binary format
     * @param value Value to serialize
     * @return Serialized binary data
     */
    static SerializedData serialize(const Value& value);
    
    /**
     * @brief Deserialize binary data to value
     * @param data Serialized data
     * @return Deserialized value
     */
    static Value deserialize(const SerializedData& data);
    
    /**
     * @brief Check if a value is cloneable
     */
    static bool isCloneable(const Value& value);

private:
    // Internal clone with cycle detection
    static Value cloneInternal(const Value& input,
                               std::unordered_map<Object*, Object*>& memory,
                               const std::vector<Object*>& transferList);
    
    // Serialization helpers
    static void writeValue(std::vector<uint8_t>& buffer, const Value& value,
                          std::unordered_map<Object*, uint32_t>& objMap);
    static Value readValue(const uint8_t* data, size_t& offset, size_t size,
                          std::vector<Object*>& objCache);
    
    // Type markers for serialization
    enum class TypeTag : uint8_t {
        Undefined = 0x00,
        Null = 0x01,
        BoolFalse = 0x02,
        BoolTrue = 0x03,
        Int32 = 0x04,
        Float64 = 0x05,
        String = 0x06,
        Object = 0x07,
        Array = 0x08,
        Date = 0x09,
        RegExp = 0x0A,
        Map = 0x0B,
        Set = 0x0C,
        ArrayBuffer = 0x0D,
        TypedArray = 0x0E,
        DataView = 0x0F,
        Error = 0x10,
        ObjectRef = 0xFF  // Circular reference
    };
};

/**
 * @brief Builtin function for globalThis.structuredClone
 */
Value structuredCloneBuiltin(void* ctx, const std::vector<Value>& args);

} // namespace Zepra::Browser
