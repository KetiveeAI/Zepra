/**
 * @file object.cpp
 * @brief JavaScript object implementation
 */

#include "zeprascript/runtime/object.hpp"
#include "zeprascript/runtime/function.hpp"
#include <algorithm>

namespace Zepra::Runtime {

// =============================================================================
// Object
// =============================================================================

Object::Object(ObjectType type)
    : objectType_(type)
{
}

Value Object::get(const std::string& key) const {
    // Check own properties
    auto it = properties_.find(key);
    if (it != properties_.end()) {
        return it->second;
    }
    
    // Check prototype chain
    if (prototype_) {
        return prototype_->get(key);
    }
    
    return Value::undefined();
}

Value Object::get(uint32_t index) const {
    if (index < elements_.size()) {
        return elements_[index];
    }
    
    // Fall back to string key
    return get(std::to_string(index));
}

bool Object::set(const std::string& key, Value value) {
    // Check if property is writable
    auto descIt = descriptors_.find(key);
    if (descIt != descriptors_.end()) {
        if (!descIt->second.isWritable()) {
            return false;
        }
    }
    
    properties_[key] = value;
    return true;
}

bool Object::set(uint32_t index, Value value) {
    if (index >= elements_.size()) {
        elements_.resize(index + 1, Value::undefined());
    }
    elements_[index] = value;
    return true;
}

bool Object::has(const std::string& key) const {
    if (properties_.find(key) != properties_.end()) {
        return true;
    }
    if (prototype_) {
        return prototype_->has(key);
    }
    return false;
}

bool Object::has(uint32_t index) const {
    if (index < elements_.size() && !elements_[index].isUndefined()) {
        return true;
    }
    return has(std::to_string(index));
}

bool Object::deleteProperty(const std::string& key) {
    auto descIt = descriptors_.find(key);
    if (descIt != descriptors_.end()) {
        if (!descIt->second.isConfigurable()) {
            return false;
        }
        descriptors_.erase(descIt);
    }
    
    properties_.erase(key);
    return true;
}

bool Object::deleteProperty(uint32_t index) {
    if (index < elements_.size()) {
        elements_[index] = Value::undefined();
        return true;
    }
    return deleteProperty(std::to_string(index));
}

bool Object::defineProperty(const std::string& key, const PropertyDescriptor& desc) {
    if (!extensible_ && properties_.find(key) == properties_.end()) {
        return false;
    }
    
    descriptors_[key] = desc;
    properties_[key] = desc.value;
    return true;
}

std::optional<PropertyDescriptor> Object::getOwnPropertyDescriptor(const std::string& key) const {
    auto it = descriptors_.find(key);
    if (it != descriptors_.end()) {
        return it->second;
    }
    
    auto propIt = properties_.find(key);
    if (propIt != properties_.end()) {
        PropertyDescriptor desc;
        desc.value = propIt->second;
        desc.attributes = PropertyAttribute::Default;
        return desc;
    }
    
    return std::nullopt;
}

std::vector<std::string> Object::getOwnPropertyNames() const {
    std::vector<std::string> names;
    names.reserve(properties_.size() + elements_.size());
    
    // Add indexed properties
    for (size_t i = 0; i < elements_.size(); i++) {
        if (!elements_[i].isUndefined()) {
            names.push_back(std::to_string(i));
        }
    }
    
    // Add named properties
    for (const auto& [key, _] : properties_) {
        names.push_back(key);
    }
    
    return names;
}

std::vector<std::string> Object::keys() const {
    std::vector<std::string> result;
    
    // Add indexed properties
    for (size_t i = 0; i < elements_.size(); i++) {
        if (!elements_[i].isUndefined()) {
            result.push_back(std::to_string(i));
        }
    }
    
    // Add enumerable named properties
    for (const auto& [key, _] : properties_) {
        auto descIt = descriptors_.find(key);
        if (descIt == descriptors_.end() || descIt->second.isEnumerable()) {
            result.push_back(key);
        }
    }
    
    return result;
}

bool Object::isCallable() const {
    return isFunction();
}

bool Object::isConstructor() const {
    if (!isFunction()) return false;
    return static_cast<const Function*>(this)->isConstructor();
}

size_t Object::length() const {
    return elements_.size();
}

void Object::setLength(size_t len) {
    elements_.resize(len, Value::undefined());
}

void Object::markGC() {
    if (gcMarked_) return;
    gcMarked_ = true;
    
    // Mark prototype
    if (prototype_) {
        prototype_->markGC();
    }
    
    // Mark properties
    for (const auto& [_, value] : properties_) {
        if (value.isObject()) {
            value.asObject()->markGC();
        }
    }
    
    // Mark elements
    for (const auto& value : elements_) {
        if (value.isObject()) {
            value.asObject()->markGC();
        }
    }
}

Value Object::internalSlot(const std::string& name) const {
    auto it = internalSlots_.find(name);
    if (it != internalSlots_.end()) {
        return it->second;
    }
    return Value::undefined();
}

void Object::setInternalSlot(const std::string& name, Value value) {
    internalSlots_[name] = value;
}

// =============================================================================
// String
// =============================================================================

// Shared String prototype that gets attached to all String instances
static Object* getStringPrototype() {
    static Object* stringProto = nullptr;
    if (!stringProto) {
        stringProto = new Object();
        
        // slice(start, end?)
        stringProto->set("slice", Value::object(
            new Function("slice", [](const FunctionCallInfo& info) -> Value {
                Value thisVal = info.thisValue();
                if (!thisVal.isString()) return Value::undefined();
                String* str = static_cast<String*>(thisVal.asObject());
                const std::string& s = str->value();
                
                int64_t len = static_cast<int64_t>(s.size());
                int64_t start = info.argumentCount() > 0 ? static_cast<int64_t>(info.argument(0).toNumber()) : 0;
                int64_t end = info.argumentCount() > 1 ? static_cast<int64_t>(info.argument(1).toNumber()) : len;
                
                if (start < 0) start = std::max(len + start, int64_t(0));
                if (end < 0) end = std::max(len + end, int64_t(0));
                if (start > len) start = len;
                if (end > len) end = len;
                if (start >= end) return Value::string(new String(""));
                
                return Value::string(new String(s.substr(static_cast<size_t>(start), static_cast<size_t>(end - start))));
            }, 2)));
        
        // toUpperCase()
        stringProto->set("toUpperCase", Value::object(
            new Function("toUpperCase", [](const FunctionCallInfo& info) -> Value {
                Value thisVal = info.thisValue();
                if (!thisVal.isString()) return Value::undefined();
                String* str = static_cast<String*>(thisVal.asObject());
                std::string result = str->value();
                std::transform(result.begin(), result.end(), result.begin(), ::toupper);
                return Value::string(new String(result));
            }, 0)));
        
        // toLowerCase()
        stringProto->set("toLowerCase", Value::object(
            new Function("toLowerCase", [](const FunctionCallInfo& info) -> Value {
                Value thisVal = info.thisValue();
                if (!thisVal.isString()) return Value::undefined();
                String* str = static_cast<String*>(thisVal.asObject());
                std::string result = str->value();
                std::transform(result.begin(), result.end(), result.begin(), ::tolower);
                return Value::string(new String(result));
            }, 0)));
        
        // trim()
        stringProto->set("trim", Value::object(
            new Function("trim", [](const FunctionCallInfo& info) -> Value {
                Value thisVal = info.thisValue();
                if (!thisVal.isString()) return Value::undefined();
                String* str = static_cast<String*>(thisVal.asObject());
                const std::string& s = str->value();
                const std::string ws = " \t\n\r\f\v";
                size_t start = s.find_first_not_of(ws);
                if (start == std::string::npos) return Value::string(new String(""));
                size_t end = s.find_last_not_of(ws);
                return Value::string(new String(s.substr(start, end - start + 1)));
            }, 0)));
        
        // charAt(index)
        stringProto->set("charAt", Value::object(
            new Function("charAt", [](const FunctionCallInfo& info) -> Value {
                Value thisVal = info.thisValue();
                if (!thisVal.isString()) return Value::undefined();
                String* str = static_cast<String*>(thisVal.asObject());
                size_t idx = info.argumentCount() > 0 ? static_cast<size_t>(info.argument(0).toNumber()) : 0;
                if (idx >= str->value().size()) return Value::string(new String(""));
                return Value::string(new String(std::string(1, str->value()[idx])));
            }, 1)));
        
        // indexOf(search, start?)
        stringProto->set("indexOf", Value::object(
            new Function("indexOf", [](const FunctionCallInfo& info) -> Value {
                Value thisVal = info.thisValue();
                if (!thisVal.isString() || info.argumentCount() < 1) return Value::number(-1);
                String* str = static_cast<String*>(thisVal.asObject());
                std::string search = info.argument(0).toString();
                size_t start = info.argumentCount() > 1 ? static_cast<size_t>(info.argument(1).toNumber()) : 0;
                size_t pos = str->value().find(search, start);
                return Value::number(pos == std::string::npos ? -1 : static_cast<double>(pos));
            }, 1)));
        
        // split(separator)
        stringProto->set("split", Value::object(
            new Function("split", [](const FunctionCallInfo& info) -> Value {
                Value thisVal = info.thisValue();
                if (!thisVal.isString()) return Value::object(new Array({}));
                String* str = static_cast<String*>(thisVal.asObject());
                const std::string& s = str->value();
                
                if (info.argumentCount() < 1) {
                    std::vector<Value> parts;
                    parts.push_back(Value::string(new String(s)));
                    return Value::object(new Array(std::move(parts)));
                }
                
                std::string sep = info.argument(0).toString();
                std::vector<Value> parts;
                
                if (sep.empty()) {
                    for (char c : s) {
                        parts.push_back(Value::string(new String(std::string(1, c))));
                    }
                } else {
                    size_t pos = 0, found;
                    while ((found = s.find(sep, pos)) != std::string::npos) {
                        parts.push_back(Value::string(new String(s.substr(pos, found - pos))));
                        pos = found + sep.size();
                    }
                    parts.push_back(Value::string(new String(s.substr(pos))));
                }
                
                return Value::object(new Array(std::move(parts)));
            }, 1)));
    }
    return stringProto;
}

String::String(std::string value)
    : Object(ObjectType::String)
    , value_(std::move(value))
{
    setPrototype(getStringPrototype());
}

Value String::charAt(size_t index) const {
    if (index >= value_.size()) {
        return Value::string(new String(""));
    }
    return Value::string(new String(std::string(1, value_[index])));
}

int String::charCodeAt(size_t index) const {
    if (index >= value_.size()) {
        return -1; // NaN in JS
    }
    return static_cast<unsigned char>(value_[index]);
}

String* String::concat(const String* other) const {
    return new String(value_ + other->value_);
}

String* String::substring(size_t start, size_t end) const {
    if (start > value_.size()) start = value_.size();
    if (end > value_.size()) end = value_.size();
    if (start > end) std::swap(start, end);
    
    return new String(value_.substr(start, end - start));
}

String* String::toLowerCase() const {
    std::string result = value_;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return new String(result);
}

String* String::toUpperCase() const {
    std::string result = value_;
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return new String(result);
}

String* String::trim() const {
    const std::string whitespace = " \t\n\r\f\v";
    size_t start = value_.find_first_not_of(whitespace);
    if (start == std::string::npos) {
        return new String("");
    }
    size_t end = value_.find_last_not_of(whitespace);
    return new String(value_.substr(start, end - start + 1));
}

int String::indexOf(const std::string& search, size_t start) const {
    if (start >= value_.size()) return -1;
    size_t pos = value_.find(search, start);
    return pos == std::string::npos ? -1 : static_cast<int>(pos);
}

int String::lastIndexOf(const std::string& search) const {
    size_t pos = value_.rfind(search);
    return pos == std::string::npos ? -1 : static_cast<int>(pos);
}

bool String::includes(const std::string& search, size_t start) const {
    return indexOf(search, start) != -1;
}

bool String::startsWith(const std::string& prefix, size_t start) const {
    if (start + prefix.size() > value_.size()) return false;
    return value_.compare(start, prefix.size(), prefix) == 0;
}

bool String::endsWith(const std::string& suffix) const {
    if (suffix.size() > value_.size()) return false;
    return value_.compare(value_.size() - suffix.size(), suffix.size(), suffix) == 0;
}

// =============================================================================
// Array
// =============================================================================

// Shared Array prototype - forward declare
namespace Zepra::Builtins { class ArrayBuiltin; }

// Shared Array prototype that gets attached to all Array instances
static Object* getArrayPrototype() {
    static Object* arrayProto = nullptr;
    if (!arrayProto) {
        // Create minimal array prototype with essential methods
        arrayProto = new Object();
        
        // push() method
        arrayProto->set("push", Value::object(
            new Function("push", [](const FunctionCallInfo& info) -> Value {
                Value thisVal = info.thisValue();
                if (!thisVal.isObject()) return Value::undefined();
                Array* arr = dynamic_cast<Array*>(thisVal.asObject());
                if (!arr) return Value::undefined();
                for (size_t i = 0; i < info.argumentCount(); i++) {
                    arr->push(info.argument(i));
                }
                return Value::number(static_cast<double>(arr->length()));
            }, 1)));
        
        // pop() method
        arrayProto->set("pop", Value::object(
            new Function("pop", [](const FunctionCallInfo& info) -> Value {
                Value thisVal = info.thisValue();
                if (!thisVal.isObject()) return Value::undefined();
                Array* arr = dynamic_cast<Array*>(thisVal.asObject());
                if (!arr || arr->length() == 0) return Value::undefined();
                return arr->pop();
            }, 0)));
        
        // length property access is handled by the Array class directly
    }
    return arrayProto;
}

Array::Array()
    : Object(ObjectType::Array)
{
    setPrototype(getArrayPrototype());
}

Array::Array(size_t length)
    : Object(ObjectType::Array)
{
    elements_.resize(length, Value::undefined());
    setPrototype(getArrayPrototype());
}

Array::Array(std::vector<Value> elements)
    : Object(ObjectType::Array)
{
    // Assign to the base class elements_
    Object::elements_ = std::move(elements);
    setPrototype(getArrayPrototype());
}

void Array::push(Value value) {
    elements_.push_back(value);
}

Value Array::pop() {
    if (elements_.empty()) {
        return Value::undefined();
    }
    Value val = elements_.back();
    elements_.pop_back();
    return val;
}

void Array::unshift(Value value) {
    elements_.insert(elements_.begin(), value);
}

Value Array::shift() {
    if (elements_.empty()) {
        return Value::undefined();
    }
    Value val = elements_.front();
    elements_.erase(elements_.begin());
    return val;
}

Value Array::at(size_t index) const {
    if (index < elements_.size()) {
        return elements_[index];
    }
    return Value::undefined();
}

void Array::splice(size_t start, size_t deleteCount, const std::vector<Value>& items) {
    if (start > elements_.size()) start = elements_.size();
    if (start + deleteCount > elements_.size()) {
        deleteCount = elements_.size() - start;
    }
    
    elements_.erase(elements_.begin() + start, 
                    elements_.begin() + start + deleteCount);
    elements_.insert(elements_.begin() + start, items.begin(), items.end());
}

Array* Array::slice(size_t start, size_t end) const {
    if (start > elements_.size()) start = elements_.size();
    if (end > elements_.size()) end = elements_.size();
    if (start > end) return new Array();
    
    std::vector<Value> result(elements_.begin() + start, elements_.begin() + end);
    return new Array(std::move(result));
}

Array* Array::concat(const Array* other) const {
    std::vector<Value> result = elements_;
    result.insert(result.end(), other->elements_.begin(), other->elements_.end());
    return new Array(std::move(result));
}

int Array::indexOf(const Value& value, size_t start) const {
    for (size_t i = start; i < elements_.size(); i++) {
        if (elements_[i].strictEquals(value)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

bool Array::includes(const Value& value, size_t start) const {
    return indexOf(value, start) != -1;
}

void Array::reverse() {
    std::reverse(elements_.begin(), elements_.end());
}

void Array::fill(Value value, size_t start, size_t end) {
    if (start >= elements_.size()) return;
    if (end > elements_.size()) end = elements_.size();
    
    for (size_t i = start; i < end; i++) {
        elements_[i] = value;
    }
}

void Array::set(size_t index, Value value) {
    if (index >= elements_.size()) {
        elements_.resize(index + 1, Value::undefined());
    }
    elements_[index] = value;
}

void Array::setLength(size_t len) {
    elements_.resize(len, Value::undefined());
}

} // namespace Zepra::Runtime

