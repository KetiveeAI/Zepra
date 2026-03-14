// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file structured_clone.cpp
 * @brief Structured Clone Algorithm implementation
 */

#include "browser/StructuredClone.h"
#include "runtime/objects/function.hpp"
#include "runtime/objects/object.hpp"
#include <cstring>
#include <cmath>

namespace Zepra::Browser {

// Simple Array wrapper for cloning purposes
namespace {
class ArrayObject : public Runtime::Object {
public:
    ArrayObject() : Object(Runtime::ObjectType::Array) {}
    
    void push(const Value& val) {
        size_t len = elements_.size();
        elements_.push_back(val);
    }
    
    size_t getLength() const {
        return elements_.size();
    }
    
    Value getAt(size_t index) const {
        if (index < elements_.size()) {
            return elements_[index];
        }
        return Value::undefined();
    }
};
}

// =============================================================================
// Clone Implementation
// =============================================================================

Value StructuredClone::clone(const Value& input, 
                              const std::vector<Object*>& transferList) {
    std::unordered_map<Object*, Object*> memory;
    return cloneInternal(input, memory, transferList);
}

Value StructuredClone::cloneInternal(const Value& input,
                                      std::unordered_map<Object*, Object*>& memory,
                                      const std::vector<Object*>& transferList) {
    // Primitives are directly copied
    if (input.isUndefined()) return Value::undefined();
    if (input.isNull()) return Value::null();
    if (input.isBoolean()) return Value::boolean(input.asBoolean());
    if (input.isNumber()) return Value::number(input.asNumber());
    
    // String - create new string with same content
    if (input.isString()) {
        auto* str = input.asString();
        return Value::string(new Runtime::String(str->value()));
    }
    
    // Objects require deep cloning
    if (input.isObject()) {
        Object* obj = input.asObject();
        
        // Check for circular reference
        auto it = memory.find(obj);
        if (it != memory.end()) {
            return Value::object(it->second);
        }
        
        // Check if object should be transferred
        for (Object* transfer : transferList) {
            if (transfer == obj) {
                // Transfer ownership - mark original as detached
                memory[obj] = obj;
                return Value::object(obj);
            }
        }
        
        // Check if function - not cloneable
        if (obj->isFunction()) {
            throw DataCloneError("Cannot clone a function");
        }
        
        // Handle Array
        if (obj->isArray()) {
            Runtime::Array* clonedArr = new Runtime::Array();
            memory[obj] = clonedArr;
            
            size_t len = obj->length();
            for (size_t i = 0; i < len; i++) {
                Value elem = obj->get(static_cast<uint32_t>(i));
                clonedArr->push(cloneInternal(elem, memory, transferList));
            }
            return Value::object(clonedArr);
        }
        
        // Handle plain Object
        Runtime::Object* clonedObj = new Runtime::Object();
        memory[obj] = clonedObj;
        
        // Copy all enumerable own properties
        for (const auto& key : obj->keys()) {
            Value val = obj->get(key);
            clonedObj->set(key, cloneInternal(val, memory, transferList));
        }
        
        return Value::object(clonedObj);
    }
    
    throw DataCloneError("Cannot clone value of this type");
}

// =============================================================================
// Serialization
// =============================================================================

bool StructuredClone::isCloneable(const Value& value) {
    if (value.isUndefined() || value.isNull() || 
        value.isBoolean() || value.isNumber() || value.isString()) {
        return true;
    }
    
    if (value.isObject()) {
        Object* obj = value.asObject();
        // Functions cannot be cloned
        if (obj->isFunction()) return false;
        return true;
    }
    
    return false;
}

// Write helpers
static void writeUint8(std::vector<uint8_t>& buf, uint8_t val) {
    buf.push_back(val);
}

static void writeUint32(std::vector<uint8_t>& buf, uint32_t val) {
    buf.push_back((val >> 24) & 0xFF);
    buf.push_back((val >> 16) & 0xFF);
    buf.push_back((val >> 8) & 0xFF);
    buf.push_back(val & 0xFF);
}

static void writeFloat64(std::vector<uint8_t>& buf, double val) {
    uint64_t bits;
    std::memcpy(&bits, &val, sizeof(bits));
    for (int i = 7; i >= 0; i--) {
        buf.push_back((bits >> (i * 8)) & 0xFF);
    }
}

static void writeString(std::vector<uint8_t>& buf, const std::string& str) {
    writeUint32(buf, static_cast<uint32_t>(str.size()));
    for (char c : str) {
        buf.push_back(static_cast<uint8_t>(c));
    }
}

// Read helpers
static uint8_t readUint8(const uint8_t* data, size_t& offset) {
    return data[offset++];
}

static uint32_t readUint32(const uint8_t* data, size_t& offset) {
    uint32_t val = 0;
    val |= static_cast<uint32_t>(data[offset++]) << 24;
    val |= static_cast<uint32_t>(data[offset++]) << 16;
    val |= static_cast<uint32_t>(data[offset++]) << 8;
    val |= static_cast<uint32_t>(data[offset++]);
    return val;
}

static double readFloat64(const uint8_t* data, size_t& offset) {
    uint64_t bits = 0;
    for (int i = 7; i >= 0; i--) {
        bits |= static_cast<uint64_t>(data[offset++]) << (i * 8);
    }
    double val;
    std::memcpy(&val, &bits, sizeof(val));
    return val;
}

static std::string readStringData(const uint8_t* data, size_t& offset) {
    uint32_t len = readUint32(data, offset);
    std::string str(reinterpret_cast<const char*>(data + offset), len);
    offset += len;
    return str;
}

void StructuredClone::writeValue(std::vector<uint8_t>& buffer, 
                                  const Value& value,
                                  std::unordered_map<Object*, uint32_t>& objMap) {
    if (value.isUndefined()) {
        writeUint8(buffer, static_cast<uint8_t>(TypeTag::Undefined));
        return;
    }
    
    if (value.isNull()) {
        writeUint8(buffer, static_cast<uint8_t>(TypeTag::Null));
        return;
    }
    
    if (value.isBoolean()) {
        writeUint8(buffer, value.asBoolean() 
            ? static_cast<uint8_t>(TypeTag::BoolTrue)
            : static_cast<uint8_t>(TypeTag::BoolFalse));
        return;
    }
    
    if (value.isNumber()) {
        double num = value.asNumber();
        int32_t intVal = static_cast<int32_t>(num);
        
        // Use Int32 for small integers
        if (num == static_cast<double>(intVal) && 
            intVal >= -2147483648 && intVal <= 2147483647) {
            writeUint8(buffer, static_cast<uint8_t>(TypeTag::Int32));
            writeUint32(buffer, static_cast<uint32_t>(intVal));
        } else {
            writeUint8(buffer, static_cast<uint8_t>(TypeTag::Float64));
            writeFloat64(buffer, num);
        }
        return;
    }
    
    if (value.isString()) {
        writeUint8(buffer, static_cast<uint8_t>(TypeTag::String));
        writeString(buffer, value.asString()->value());
        return;
    }
    
    if (value.isObject()) {
        Object* obj = value.asObject();
        
        // Check for circular reference
        auto it = objMap.find(obj);
        if (it != objMap.end()) {
            writeUint8(buffer, static_cast<uint8_t>(TypeTag::ObjectRef));
            writeUint32(buffer, it->second);
            return;
        }
        
        uint32_t objIndex = static_cast<uint32_t>(objMap.size());
        objMap[obj] = objIndex;
        
        // Handle Array
        if (obj->isArray()) {
            Runtime::Array* arr = static_cast<Runtime::Array*>(obj);
            writeUint8(buffer, static_cast<uint8_t>(TypeTag::Array));
            
            size_t len = arr->length();
            writeUint32(buffer, static_cast<uint32_t>(len));
            
            for (size_t i = 0; i < len; i++) {
                writeValue(buffer, arr->at(i), objMap);
            }
            return;
        }
        
        // Plain Object
        writeUint8(buffer, static_cast<uint8_t>(TypeTag::Object));
        
        auto keys = obj->keys();
        writeUint32(buffer, static_cast<uint32_t>(keys.size()));
        
        for (const auto& key : keys) {
            writeString(buffer, key);
            writeValue(buffer, obj->get(key), objMap);
        }
        return;
    }
    
    // Fallback to undefined for unsupported types
    writeUint8(buffer, static_cast<uint8_t>(TypeTag::Undefined));
}

Value StructuredClone::readValue(const uint8_t* data, size_t& offset, 
                                  size_t size, std::vector<Object*>& objCache) {
    if (offset >= size) {
        throw DataCloneError("Unexpected end of data");
    }
    
    TypeTag tag = static_cast<TypeTag>(readUint8(data, offset));
    
    switch (tag) {
        case TypeTag::Undefined:
            return Value::undefined();
            
        case TypeTag::Null:
            return Value::null();
            
        case TypeTag::BoolFalse:
            return Value::boolean(false);
            
        case TypeTag::BoolTrue:
            return Value::boolean(true);
            
        case TypeTag::Int32: {
            int32_t val = static_cast<int32_t>(readUint32(data, offset));
            return Value::number(static_cast<double>(val));
        }
        
        case TypeTag::Float64: {
            double val = readFloat64(data, offset);
            return Value::number(val);
        }
        
        case TypeTag::String: {
            std::string str = readStringData(data, offset);
            return Value::string(new Runtime::String(str));
        }
        
        case TypeTag::Array: {
            uint32_t len = readUint32(data, offset);
            Runtime::Array* arr = new Runtime::Array();
            objCache.push_back(arr);
            
            for (uint32_t i = 0; i < len; i++) {
                arr->push(readValue(data, offset, size, objCache));
            }
            return Value::object(arr);
        }
        
        case TypeTag::Object: {
            uint32_t propCount = readUint32(data, offset);
            Runtime::Object* obj = new Runtime::Object();
            objCache.push_back(obj);
            
            for (uint32_t i = 0; i < propCount; i++) {
                std::string key = readStringData(data, offset);
                Value val = readValue(data, offset, size, objCache);
                obj->set(key, val);
            }
            return Value::object(obj);
        }
        
        case TypeTag::ObjectRef: {
            uint32_t idx = readUint32(data, offset);
            if (idx >= objCache.size()) {
                throw DataCloneError("Invalid object reference");
            }
            return Value::object(objCache[idx]);
        }
        
        default:
            throw DataCloneError("Unknown type tag in serialized data");
    }
}

SerializedData StructuredClone::serialize(const Value& value) {
    SerializedData result;
    std::unordered_map<Object*, uint32_t> objMap;
    
    writeValue(result.buffer, value, objMap);
    
    return result;
}

Value StructuredClone::deserialize(const SerializedData& data) {
    if (data.buffer.empty()) {
        return Value::undefined();
    }
    
    std::vector<Object*> objCache;
    size_t offset = 0;
    
    return readValue(data.buffer.data(), offset, data.buffer.size(), objCache);
}

// =============================================================================
// Builtin Function
// =============================================================================

Value structuredCloneBuiltin(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty()) {
        return Value::undefined();
    }
    
    std::vector<Object*> transferList;
    
    // Check for options object with transfer array
    if (args.size() > 1 && args[1].isObject()) {
        Object* opts = args[1].asObject();
        Value transfer = opts->get("transfer");
        if (transfer.isObject() && transfer.asObject()->isArray()) {
            Runtime::Array* arr = static_cast<Runtime::Array*>(transfer.asObject());
            for (size_t i = 0; i < arr->length(); i++) {
                Value elem = arr->at(i);
                if (elem.isObject()) {
                    transferList.push_back(elem.asObject());
                }
            }
        }
    }
    
    return StructuredClone::clone(args[0], transferList);
}

} // namespace Zepra::Browser
