/**
 * @file NativeBinding.h
 * @brief Native C++ to JavaScript binding infrastructure
 * 
 * Implements:
 * - Type conversion helpers
 * - Argument extraction
 * - Return value wrapping
 * - Exception handling
 * 
 * For implementing builtins efficiently
 */

#pragma once

#include "value.hpp"
#include "object.hpp"
#include <functional>
#include <string>
#include <vector>
#include <optional>
#include <tuple>
#include <type_traits>

namespace Zepra::Runtime {

// Forward declarations
class VM;

// =============================================================================
// Call Info
// =============================================================================

/**
 * @brief Information about a native function call
 */
struct CallInfo {
    VM* vm;
    Value thisValue;
    const Value* arguments;
    size_t argumentCount;
    bool isConstructCall;
    
    Value argument(size_t index) const {
        return index < argumentCount ? arguments[index] : Value::undefined();
    }
    
    bool hasArgument(size_t index) const {
        return index < argumentCount && !arguments[index].isUndefined();
    }
};

// =============================================================================
// Type Conversion Traits
// =============================================================================

template<typename T>
struct ValueConverter {
    static T from(const Value& v);
    static Value to(const T& v);
};

// Specializations
template<>
struct ValueConverter<bool> {
    static bool from(const Value& v) { return v.toBoolean(); }
    static Value to(bool v) { return Value::boolean(v); }
};

template<>
struct ValueConverter<int32_t> {
    static int32_t from(const Value& v) { return v.toInt32(); }
    static Value to(int32_t v) { return Value::number(v); }
};

template<>
struct ValueConverter<uint32_t> {
    static uint32_t from(const Value& v) { return v.toUint32(); }
    static Value to(uint32_t v) { return Value::number(v); }
};

template<>
struct ValueConverter<int64_t> {
    static int64_t from(const Value& v) { return static_cast<int64_t>(v.toNumber()); }
    static Value to(int64_t v) { return Value::number(static_cast<double>(v)); }
};

template<>
struct ValueConverter<double> {
    static double from(const Value& v) { return v.toNumber(); }
    static Value to(double v) { return Value::number(v); }
};

template<>
struct ValueConverter<std::string> {
    static std::string from(const Value& v) { return v.toString(); }
    static Value to(const std::string& v) { return Value::string(v); }
};

template<>
struct ValueConverter<Value> {
    static Value from(const Value& v) { return v; }
    static Value to(const Value& v) { return v; }
};

// Optional support
template<typename T>
struct ValueConverter<std::optional<T>> {
    static std::optional<T> from(const Value& v) {
        if (v.isUndefined() || v.isNull()) return std::nullopt;
        return ValueConverter<T>::from(v);
    }
    static Value to(const std::optional<T>& v) {
        if (!v) return Value::undefined();
        return ValueConverter<T>::to(*v);
    }
};

// =============================================================================
// Argument Extractor
// =============================================================================

template<typename... Args>
class ArgumentExtractor {
public:
    static constexpr size_t ARITY = sizeof...(Args);
    
    template<size_t I>
    using ArgType = std::tuple_element_t<I, std::tuple<Args...>>;
    
    static std::tuple<Args...> extract(const CallInfo& info) {
        return extractImpl(info, std::index_sequence_for<Args...>{});
    }
    
private:
    template<size_t... Is>
    static std::tuple<Args...> extractImpl(const CallInfo& info, std::index_sequence<Is...>) {
        return std::make_tuple(
            ValueConverter<std::decay_t<Args>>::from(info.argument(Is))...
        );
    }
};

// =============================================================================
// Native Function Wrapper
// =============================================================================

using NativeFunction = std::function<Value(const CallInfo&)>;

/**
 * @brief Wrap a C++ function as a native JS function
 */
template<typename Ret, typename... Args>
NativeFunction wrapFunction(Ret (*fn)(Args...)) {
    return [fn](const CallInfo& info) -> Value {
        auto args = ArgumentExtractor<Args...>::extract(info);
        if constexpr (std::is_void_v<Ret>) {
            std::apply(fn, args);
            return Value::undefined();
        } else {
            return ValueConverter<Ret>::to(std::apply(fn, args));
        }
    };
}

/**
 * @brief Wrap a member function
 */
template<typename T, typename Ret, typename... Args>
NativeFunction wrapMethod(Ret (T::*method)(Args...)) {
    return [method](const CallInfo& info) -> Value {
        T* obj = info.thisValue.template as<T>();
        if (!obj) return Value::undefined();
        
        auto args = ArgumentExtractor<Args...>::extract(info);
        if constexpr (std::is_void_v<Ret>) {
            (obj->*method)(std::get<Args>(args)...);
            return Value::undefined();
        } else {
            return ValueConverter<Ret>::to((obj->*method)(std::get<Args>(args)...));
        }
    };
}

// =============================================================================
// Builtin Function Builder
// =============================================================================

class BuiltinBuilder {
public:
    explicit BuiltinBuilder(const std::string& name) : name_(name) {}
    
    BuiltinBuilder& length(uint32_t len) {
        length_ = len;
        return *this;
    }
    
    BuiltinBuilder& function(NativeFunction fn) {
        function_ = std::move(fn);
        return *this;
    }
    
    BuiltinBuilder& constructor() {
        isConstructor_ = true;
        return *this;
    }
    
    BuiltinBuilder& getter() {
        isGetter_ = true;
        return *this;
    }
    
    BuiltinBuilder& setter() {
        isSetter_ = true;
        return *this;
    }
    
    // Build the function object
    Value build(VM* vm);
    
    // Properties
    const std::string& name() const { return name_; }
    bool isConstructor() const { return isConstructor_; }
    
private:
    std::string name_;
    uint32_t length_ = 0;
    NativeFunction function_;
    bool isConstructor_ = false;
    bool isGetter_ = false;
    bool isSetter_ = false;
};

// =============================================================================
// Prototype Builder
// =============================================================================

class PrototypeBuilder {
public:
    explicit PrototypeBuilder(const std::string& className) : className_(className) {}
    
    PrototypeBuilder& method(const std::string& name, NativeFunction fn, uint32_t length = 0);
    PrototypeBuilder& getter(const std::string& name, NativeFunction fn);
    PrototypeBuilder& setter(const std::string& name, NativeFunction fn);
    PrototypeBuilder& accessor(const std::string& name, NativeFunction getter, NativeFunction setter);
    PrototypeBuilder& constant(const std::string& name, Value value);
    PrototypeBuilder& staticMethod(const std::string& name, NativeFunction fn, uint32_t length = 0);
    
    // Build constructor and prototype
    struct BuiltClass {
        Value constructor;
        Value prototype;
    };
    
    BuiltClass build(VM* vm);
    
private:
    struct MethodDef {
        std::string name;
        NativeFunction fn;
        uint32_t length;
        bool isStatic;
    };
    
    struct AccessorDef {
        std::string name;
        NativeFunction getter;
        NativeFunction setter;
    };
    
    struct ConstantDef {
        std::string name;
        Value value;
    };
    
    std::string className_;
    std::vector<MethodDef> methods_;
    std::vector<AccessorDef> accessors_;
    std::vector<ConstantDef> constants_;
};

// =============================================================================
// Type Check Macros
// =============================================================================

#define REQUIRE_OBJECT(info, index) \
    do { \
        if (!(info).argument(index).isObject()) { \
            return throwTypeError((info).vm, "Expected object"); \
        } \
    } while(0)

#define REQUIRE_FUNCTION(info, index) \
    do { \
        if (!(info).argument(index).isFunction()) { \
            return throwTypeError((info).vm, "Expected function"); \
        } \
    } while(0)

#define REQUIRE_STRING(info, index) \
    do { \
        if (!(info).argument(index).isString()) { \
            return throwTypeError((info).vm, "Expected string"); \
        } \
    } while(0)

#define REQUIRE_NUMBER(info, index) \
    do { \
        if (!(info).argument(index).isNumber()) { \
            return throwTypeError((info).vm, "Expected number"); \
        } \
    } while(0)

// =============================================================================
// Error Helpers
// =============================================================================

Value throwTypeError(VM* vm, const std::string& message);
Value throwRangeError(VM* vm, const std::string& message);
Value throwReferenceError(VM* vm, const std::string& message);

} // namespace Zepra::Runtime
