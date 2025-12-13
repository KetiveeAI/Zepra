/**
 * @file data_view.cpp
 * @brief JavaScript DataView implementation
 */

#include "zeprascript/builtins/data_view.hpp"
#include "zeprascript/runtime/function.hpp"
#include <cstring>

namespace Zepra::Builtins {

DataViewObject::DataViewObject(ArrayBufferObject* buffer, size_t byteOffset, size_t byteLength)
    : Object(Runtime::ObjectType::Ordinary)
    , buffer_(buffer)
    , byteOffset_(byteOffset)
    , byteLength_(byteLength == 0 && buffer ? buffer->byteLength() - byteOffset : byteLength) {}

// Helper to swap bytes for endianness
template<typename T>
static T swapBytes(T value) {
    T result = 0;
    uint8_t* src = reinterpret_cast<uint8_t*>(&value);
    uint8_t* dst = reinterpret_cast<uint8_t*>(&result);
    for (size_t i = 0; i < sizeof(T); ++i) {
        dst[i] = src[sizeof(T) - 1 - i];
    }
    return result;
}

template<typename T>
T DataViewObject::readValue(size_t offset, bool littleEndian) const {
    if (!buffer_ || offset + sizeof(T) > byteLength_) return T{};
    
    T value;
    std::memcpy(&value, buffer_->data() + byteOffset_ + offset, sizeof(T));
    
    // Check system endianness and swap if needed
    static const uint16_t test = 1;
    bool systemIsLittleEndian = (*reinterpret_cast<const uint8_t*>(&test) == 1);
    
    if (littleEndian != systemIsLittleEndian && sizeof(T) > 1) {
        value = swapBytes(value);
    }
    
    return value;
}

template<typename T>
void DataViewObject::writeValue(size_t offset, T value, bool littleEndian) {
    if (!buffer_ || offset + sizeof(T) > byteLength_) return;
    
    static const uint16_t test = 1;
    bool systemIsLittleEndian = (*reinterpret_cast<const uint8_t*>(&test) == 1);
    
    if (littleEndian != systemIsLittleEndian && sizeof(T) > 1) {
        value = swapBytes(value);
    }
    
    std::memcpy(buffer_->data() + byteOffset_ + offset, &value, sizeof(T));
}

// Getters
int8_t DataViewObject::getInt8(size_t offset) const {
    return readValue<int8_t>(offset, true);
}

uint8_t DataViewObject::getUint8(size_t offset) const {
    return readValue<uint8_t>(offset, true);
}

int16_t DataViewObject::getInt16(size_t offset, bool littleEndian) const {
    return readValue<int16_t>(offset, littleEndian);
}

uint16_t DataViewObject::getUint16(size_t offset, bool littleEndian) const {
    return readValue<uint16_t>(offset, littleEndian);
}

int32_t DataViewObject::getInt32(size_t offset, bool littleEndian) const {
    return readValue<int32_t>(offset, littleEndian);
}

uint32_t DataViewObject::getUint32(size_t offset, bool littleEndian) const {
    return readValue<uint32_t>(offset, littleEndian);
}

float DataViewObject::getFloat32(size_t offset, bool littleEndian) const {
    return readValue<float>(offset, littleEndian);
}

double DataViewObject::getFloat64(size_t offset, bool littleEndian) const {
    return readValue<double>(offset, littleEndian);
}

// Setters
void DataViewObject::setInt8(size_t offset, int8_t value) {
    writeValue(offset, value, true);
}

void DataViewObject::setUint8(size_t offset, uint8_t value) {
    writeValue(offset, value, true);
}

void DataViewObject::setInt16(size_t offset, int16_t value, bool littleEndian) {
    writeValue(offset, value, littleEndian);
}

void DataViewObject::setUint16(size_t offset, uint16_t value, bool littleEndian) {
    writeValue(offset, value, littleEndian);
}

void DataViewObject::setInt32(size_t offset, int32_t value, bool littleEndian) {
    writeValue(offset, value, littleEndian);
}

void DataViewObject::setUint32(size_t offset, uint32_t value, bool littleEndian) {
    writeValue(offset, value, littleEndian);
}

void DataViewObject::setFloat32(size_t offset, float value, bool littleEndian) {
    writeValue(offset, value, littleEndian);
}

void DataViewObject::setFloat64(size_t offset, double value, bool littleEndian) {
    writeValue(offset, value, littleEndian);
}

// =============================================================================
// DataViewBuiltin Implementation
// =============================================================================

Value DataViewBuiltin::constructor(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isObject()) return Value::undefined();
    
    ArrayBufferObject* buffer = dynamic_cast<ArrayBufferObject*>(args[0].asObject());
    if (!buffer) return Value::undefined();
    
    size_t offset = args.size() > 1 && args[1].isNumber() 
        ? static_cast<size_t>(args[1].asNumber()) : 0;
    size_t length = args.size() > 2 && args[2].isNumber()
        ? static_cast<size_t>(args[2].asNumber()) : 0;
    
    return Value::object(new DataViewObject(buffer, offset, length));
}

Value DataViewBuiltin::getInt8(Runtime::Context*, const std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isObject() || !args[1].isNumber()) 
        return Value::undefined();
    DataViewObject* dv = dynamic_cast<DataViewObject*>(args[0].asObject());
    if (!dv) return Value::undefined();
    return Value::number(dv->getInt8(static_cast<size_t>(args[1].asNumber())));
}

Value DataViewBuiltin::getUint8(Runtime::Context*, const std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isObject() || !args[1].isNumber()) 
        return Value::undefined();
    DataViewObject* dv = dynamic_cast<DataViewObject*>(args[0].asObject());
    if (!dv) return Value::undefined();
    return Value::number(dv->getUint8(static_cast<size_t>(args[1].asNumber())));
}

Value DataViewBuiltin::getInt16(Runtime::Context*, const std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isObject() || !args[1].isNumber()) 
        return Value::undefined();
    DataViewObject* dv = dynamic_cast<DataViewObject*>(args[0].asObject());
    if (!dv) return Value::undefined();
    bool le = args.size() > 2 && args[2].isBoolean() && args[2].asBoolean();
    return Value::number(dv->getInt16(static_cast<size_t>(args[1].asNumber()), le));
}

Value DataViewBuiltin::getUint16(Runtime::Context*, const std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isObject() || !args[1].isNumber()) 
        return Value::undefined();
    DataViewObject* dv = dynamic_cast<DataViewObject*>(args[0].asObject());
    if (!dv) return Value::undefined();
    bool le = args.size() > 2 && args[2].isBoolean() && args[2].asBoolean();
    return Value::number(dv->getUint16(static_cast<size_t>(args[1].asNumber()), le));
}

Value DataViewBuiltin::getInt32(Runtime::Context*, const std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isObject() || !args[1].isNumber()) 
        return Value::undefined();
    DataViewObject* dv = dynamic_cast<DataViewObject*>(args[0].asObject());
    if (!dv) return Value::undefined();
    bool le = args.size() > 2 && args[2].isBoolean() && args[2].asBoolean();
    return Value::number(dv->getInt32(static_cast<size_t>(args[1].asNumber()), le));
}

Value DataViewBuiltin::getUint32(Runtime::Context*, const std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isObject() || !args[1].isNumber()) 
        return Value::undefined();
    DataViewObject* dv = dynamic_cast<DataViewObject*>(args[0].asObject());
    if (!dv) return Value::undefined();
    bool le = args.size() > 2 && args[2].isBoolean() && args[2].asBoolean();
    return Value::number(dv->getUint32(static_cast<size_t>(args[1].asNumber()), le));
}

Value DataViewBuiltin::getFloat32(Runtime::Context*, const std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isObject() || !args[1].isNumber()) 
        return Value::undefined();
    DataViewObject* dv = dynamic_cast<DataViewObject*>(args[0].asObject());
    if (!dv) return Value::undefined();
    bool le = args.size() > 2 && args[2].isBoolean() && args[2].asBoolean();
    return Value::number(dv->getFloat32(static_cast<size_t>(args[1].asNumber()), le));
}

Value DataViewBuiltin::getFloat64(Runtime::Context*, const std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isObject() || !args[1].isNumber()) 
        return Value::undefined();
    DataViewObject* dv = dynamic_cast<DataViewObject*>(args[0].asObject());
    if (!dv) return Value::undefined();
    bool le = args.size() > 2 && args[2].isBoolean() && args[2].asBoolean();
    return Value::number(dv->getFloat64(static_cast<size_t>(args[1].asNumber()), le));
}

Object* DataViewBuiltin::createDataViewPrototype() {
    return new Object();
}

} // namespace Zepra::Builtins
