#pragma once

/**
 * @file typed_array.hpp
 * @brief JavaScript TypedArray builtins
 */

#include "../config.hpp"
#include "runtime/objects/object.hpp"
#include "runtime/objects/value.hpp"
#include <vector>
#include <cstdint>

namespace Zepra::Runtime { class Context; }

namespace Zepra::Builtins {

using Runtime::Value;
using Runtime::Object;
using Runtime::ObjectType;

/**
 * @brief ArrayBuffer - raw binary data container
 */
class ArrayBufferObject : public Object {
public:
    explicit ArrayBufferObject(size_t byteLength);
    
    uint8_t* data() { return data_.data(); }
    const uint8_t* data() const { return data_.data(); }
    size_t byteLength() const { return data_.size(); }
    
    void slice(size_t begin, size_t end, ArrayBufferObject* target) const;
    
private:
    std::vector<uint8_t> data_;
};

/**
 * @brief TypedArray element types
 */
enum class TypedArrayType : uint8_t {
    Int8,
    Uint8,
    Uint8Clamped,
    Int16,
    Uint16,
    Int32,
    Uint32,
    Float32,
    Float64,
    BigInt64,
    BigUint64
};

/**
 * @brief Base TypedArray - view into ArrayBuffer
 */
class TypedArrayObject : public Object {
public:
    TypedArrayObject(ArrayBufferObject* buffer, TypedArrayType type,
                     size_t byteOffset = 0, size_t length = 0);
    
    // TypedArray API
    Value get(size_t index) const;
    void set(size_t index, Value value);
    size_t length() const override { return length_; }
    size_t byteLength() const { return length_ * bytesPerElement(); }
    size_t byteOffset() const { return byteOffset_; }
    size_t bytesPerElement() const;
    
    ArrayBufferObject* buffer() const { return buffer_; }
    TypedArrayType type() const { return type_; }
    
    // Subarray view
    TypedArrayObject* subarray(size_t begin, size_t end) const;
    
private:
    ArrayBufferObject* buffer_;
    TypedArrayType type_;
    size_t byteOffset_;
    size_t length_;
};

/**
 * @brief TypedArray builtin functions
 */
class TypedArrayBuiltin {
public:
    // ArrayBuffer
    static Value arrayBufferConstructor(Runtime::Context* ctx, const std::vector<Value>& args);
    
    // TypedArray constructors
    static Value int8ArrayConstructor(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value uint8ArrayConstructor(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value int16ArrayConstructor(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value uint16ArrayConstructor(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value int32ArrayConstructor(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value uint32ArrayConstructor(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value float32ArrayConstructor(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value float64ArrayConstructor(Runtime::Context* ctx, const std::vector<Value>& args);
    
    static Object* createTypedArrayPrototype();
};

} // namespace Zepra::Builtins
