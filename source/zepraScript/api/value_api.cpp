/**
 * @file value_api.cpp
 * @brief Value creation and manipulation API
 */

#include "runtime/objects/value.hpp"
#include "runtime/objects/object.hpp"
#include <memory>
#include <string>

namespace Zepra {

/**
 * @brief Value factory - creates JavaScript values from C++ types
 */
class ValueFactory {
public:
    static Runtime::Value undefined() { return Runtime::Value::undefined(); }
    static Runtime::Value null() { return Runtime::Value::null(); }
    static Runtime::Value boolean(bool value) { return Runtime::Value::boolean(value); }
    static Runtime::Value number(double value) { return Runtime::Value::number(value); }
    static Runtime::Value integer(int32_t value) { return Runtime::Value::number(static_cast<double>(value)); }
    
    static Runtime::Value string(const std::string& str) {
        return Runtime::Value::string(new Runtime::String(str));
    }
    
    static Runtime::Value string(const char* str) {
        return Runtime::Value::string(new Runtime::String(str ? str : ""));
    }
    
    static Runtime::Value object() {
        return Runtime::Value::object(new Runtime::Object());
    }
    
    static Runtime::Value array(size_t length = 0) {
        return Runtime::Value::object(new Runtime::Array(length));
    }
};

/**
 * @brief Value inspector - type checking and conversion
 */
class ValueInspector {
public:
    static bool isUndefined(const Runtime::Value& v) { return v.isUndefined(); }
    static bool isNull(const Runtime::Value& v) { return v.isNull(); }
    static bool isNullOrUndefined(const Runtime::Value& v) { return v.isNull() || v.isUndefined(); }
    static bool isBoolean(const Runtime::Value& v) { return v.isBoolean(); }
    static bool isNumber(const Runtime::Value& v) { return v.isNumber(); }
    static bool isString(const Runtime::Value& v) { return v.isString(); }
    static bool isObject(const Runtime::Value& v) { return v.isObject(); }
    
    static bool isArray(const Runtime::Value& v) { 
        if (!v.isObject()) return false;
        return dynamic_cast<Runtime::Array*>(v.asObject()) != nullptr;
    }
    
    static std::string typeOf(const Runtime::Value& v) {
        if (v.isUndefined()) return "undefined";
        if (v.isNull()) return "object";
        if (v.isBoolean()) return "boolean";
        if (v.isNumber()) return "number";
        if (v.isString()) return "string";
        if (v.isObject()) return "object";
        return "undefined";
    }
    
    static bool toBoolean(const Runtime::Value& v) { return v.toBoolean(); }
    static double toNumber(const Runtime::Value& v) { return v.toNumber(); }
    static int32_t toInt32(const Runtime::Value& v) { return static_cast<int32_t>(v.toNumber()); }
    static uint32_t toUint32(const Runtime::Value& v) { return static_cast<uint32_t>(v.toNumber()); }
    static std::string toString(const Runtime::Value& v) { return v.toString(); }
};

/**
 * @brief Object utilities
 */
class ObjectUtils {
public:
    static bool hasProperty(Runtime::Object* obj, const std::string& key) {
        return obj->has(key);
    }
    
    static Runtime::Value getProperty(Runtime::Object* obj, const std::string& key) {
        return obj->get(key);
    }
    
    static bool setProperty(Runtime::Object* obj, const std::string& key, Runtime::Value value) {
        return obj->set(key, value);
    }
    
    static bool deleteProperty(Runtime::Object* obj, const std::string& key) {
        return obj->deleteProperty(key);
    }
    
    static std::vector<std::string> getOwnPropertyNames(Runtime::Object* obj) {
        return obj->keys();
    }
    
    static Runtime::Object* getPrototype(Runtime::Object* obj) {
        return obj->prototype();
    }
    
    static bool setPrototype(Runtime::Object* obj, Runtime::Object* proto) {
        obj->setPrototype(proto);
        return true;
    }
};

} // namespace Zepra
