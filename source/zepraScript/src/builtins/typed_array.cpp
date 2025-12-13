/**
 * @file typed_array.cpp
 * @brief JavaScript TypedArray builtin implementation
 */

#include "zeprascript/builtins/typed_array.hpp"
#include "zeprascript/runtime/function.hpp"
#include <cstring>

namespace Zepra::Builtins {

// =============================================================================
// ArrayBufferObject Implementation
// =============================================================================

ArrayBufferObject::ArrayBufferObject(size_t byteLength)
    : Object(ObjectType::ArrayBuffer)
    , data_(byteLength, 0) {}

void ArrayBufferObject::slice(size_t begin, size_t end, ArrayBufferObject* target) const {
    if (begin >= data_.size()) return;
    if (end > data_.size()) end = data_.size();
    if (begin >= end) return;
    
    size_t len = end - begin;
    if (target->byteLength() >= len) {
        std::memcpy(target->data(), data_.data() + begin, len);
    }
}

// =============================================================================
// TypedArrayObject Implementation
// =============================================================================

TypedArrayObject::TypedArrayObject(ArrayBufferObject* buffer, TypedArrayType type,
                                   size_t byteOffset, size_t length)
    : Object(ObjectType::TypedArray)
    , buffer_(buffer)
    , type_(type)
    , byteOffset_(byteOffset)
    , length_(length) {
    
    // If length is 0, calculate from buffer
    if (length_ == 0 && buffer_) {
        length_ = (buffer_->byteLength() - byteOffset_) / bytesPerElement();
    }
}

size_t TypedArrayObject::bytesPerElement() const {
    switch (type_) {
        case TypedArrayType::Int8:
        case TypedArrayType::Uint8:
        case TypedArrayType::Uint8Clamped:
            return 1;
        case TypedArrayType::Int16:
        case TypedArrayType::Uint16:
            return 2;
        case TypedArrayType::Int32:
        case TypedArrayType::Uint32:
        case TypedArrayType::Float32:
            return 4;
        case TypedArrayType::Float64:
        case TypedArrayType::BigInt64:
        case TypedArrayType::BigUint64:
            return 8;
    }
    return 1;
}

Value TypedArrayObject::get(size_t index) const {
    if (!buffer_ || index >= length_) return Value::undefined();
    
    uint8_t* ptr = buffer_->data() + byteOffset_ + index * bytesPerElement();
    
    switch (type_) {
        case TypedArrayType::Int8:
            return Value::number(static_cast<double>(*reinterpret_cast<int8_t*>(ptr)));
        case TypedArrayType::Uint8:
        case TypedArrayType::Uint8Clamped:
            return Value::number(static_cast<double>(*ptr));
        case TypedArrayType::Int16:
            return Value::number(static_cast<double>(*reinterpret_cast<int16_t*>(ptr)));
        case TypedArrayType::Uint16:
            return Value::number(static_cast<double>(*reinterpret_cast<uint16_t*>(ptr)));
        case TypedArrayType::Int32:
            return Value::number(static_cast<double>(*reinterpret_cast<int32_t*>(ptr)));
        case TypedArrayType::Uint32:
            return Value::number(static_cast<double>(*reinterpret_cast<uint32_t*>(ptr)));
        case TypedArrayType::Float32:
            return Value::number(static_cast<double>(*reinterpret_cast<float*>(ptr)));
        case TypedArrayType::Float64:
            return Value::number(*reinterpret_cast<double*>(ptr));
        default:
            return Value::undefined();
    }
}

void TypedArrayObject::set(size_t index, Value value) {
    if (!buffer_ || index >= length_ || !value.isNumber()) return;
    
    double num = value.asNumber();
    uint8_t* ptr = buffer_->data() + byteOffset_ + index * bytesPerElement();
    
    switch (type_) {
        case TypedArrayType::Int8:
            *reinterpret_cast<int8_t*>(ptr) = static_cast<int8_t>(num);
            break;
        case TypedArrayType::Uint8:
            *ptr = static_cast<uint8_t>(num);
            break;
        case TypedArrayType::Uint8Clamped:
            *ptr = num < 0 ? 0 : (num > 255 ? 255 : static_cast<uint8_t>(num));
            break;
        case TypedArrayType::Int16:
            *reinterpret_cast<int16_t*>(ptr) = static_cast<int16_t>(num);
            break;
        case TypedArrayType::Uint16:
            *reinterpret_cast<uint16_t*>(ptr) = static_cast<uint16_t>(num);
            break;
        case TypedArrayType::Int32:
            *reinterpret_cast<int32_t*>(ptr) = static_cast<int32_t>(num);
            break;
        case TypedArrayType::Uint32:
            *reinterpret_cast<uint32_t*>(ptr) = static_cast<uint32_t>(num);
            break;
        case TypedArrayType::Float32:
            *reinterpret_cast<float*>(ptr) = static_cast<float>(num);
            break;
        case TypedArrayType::Float64:
            *reinterpret_cast<double*>(ptr) = num;
            break;
        default:
            break;
    }
}

TypedArrayObject* TypedArrayObject::subarray(size_t begin, size_t end) const {
    if (begin >= length_) begin = length_;
    if (end > length_) end = length_;
    if (begin > end) begin = end;
    
    return new TypedArrayObject(buffer_, type_, 
                                byteOffset_ + begin * bytesPerElement(),
                                end - begin);
}

// =============================================================================
// TypedArrayBuiltin Implementation
// =============================================================================

Value TypedArrayBuiltin::arrayBufferConstructor(Runtime::Context*, const std::vector<Value>& args) {
    size_t byteLength = 0;
    if (!args.empty() && args[0].isNumber()) {
        byteLength = static_cast<size_t>(args[0].asNumber());
    }
    return Value::object(new ArrayBufferObject(byteLength));
}

static Value createTypedArray(TypedArrayType type, const std::vector<Value>& args) {
    if (args.empty()) {
        return Value::object(new TypedArrayObject(new ArrayBufferObject(0), type));
    }
    
    if (args[0].isNumber()) {
        // new Int8Array(length)
        size_t length = static_cast<size_t>(args[0].asNumber());
        size_t bytesPerElement = 1;
        switch (type) {
            case TypedArrayType::Int16:
            case TypedArrayType::Uint16:
                bytesPerElement = 2; break;
            case TypedArrayType::Int32:
            case TypedArrayType::Uint32:
            case TypedArrayType::Float32:
                bytesPerElement = 4; break;
            case TypedArrayType::Float64:
                bytesPerElement = 8; break;
            default: break;
        }
        auto* buffer = new ArrayBufferObject(length * bytesPerElement);
        return Value::object(new TypedArrayObject(buffer, type, 0, length));
    }
    
    if (args[0].isObject()) {
        ArrayBufferObject* buffer = dynamic_cast<ArrayBufferObject*>(args[0].asObject());
        if (buffer) {
            size_t offset = args.size() > 1 && args[1].isNumber() 
                ? static_cast<size_t>(args[1].asNumber()) : 0;
            size_t length = args.size() > 2 && args[2].isNumber()
                ? static_cast<size_t>(args[2].asNumber()) : 0;
            return Value::object(new TypedArrayObject(buffer, type, offset, length));
        }
    }
    
    return Value::undefined();
}

Value TypedArrayBuiltin::int8ArrayConstructor(Runtime::Context*, const std::vector<Value>& args) {
    return createTypedArray(TypedArrayType::Int8, args);
}

Value TypedArrayBuiltin::uint8ArrayConstructor(Runtime::Context*, const std::vector<Value>& args) {
    return createTypedArray(TypedArrayType::Uint8, args);
}

Value TypedArrayBuiltin::int16ArrayConstructor(Runtime::Context*, const std::vector<Value>& args) {
    return createTypedArray(TypedArrayType::Int16, args);
}

Value TypedArrayBuiltin::uint16ArrayConstructor(Runtime::Context*, const std::vector<Value>& args) {
    return createTypedArray(TypedArrayType::Uint16, args);
}

Value TypedArrayBuiltin::int32ArrayConstructor(Runtime::Context*, const std::vector<Value>& args) {
    return createTypedArray(TypedArrayType::Int32, args);
}

Value TypedArrayBuiltin::uint32ArrayConstructor(Runtime::Context*, const std::vector<Value>& args) {
    return createTypedArray(TypedArrayType::Uint32, args);
}

Value TypedArrayBuiltin::float32ArrayConstructor(Runtime::Context*, const std::vector<Value>& args) {
    return createTypedArray(TypedArrayType::Float32, args);
}

Value TypedArrayBuiltin::float64ArrayConstructor(Runtime::Context*, const std::vector<Value>& args) {
    return createTypedArray(TypedArrayType::Float64, args);
}

Object* TypedArrayBuiltin::createTypedArrayPrototype() {
    return new Object();
}

} // namespace Zepra::Builtins
