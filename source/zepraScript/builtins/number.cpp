/**
 * @file number.cpp
 * @brief JavaScript Number object implementation
 */

#include "builtins/number.hpp"
#include "runtime/objects/function.hpp"
#include <sstream>
#include <iomanip>
#include <cstdlib>

namespace Zepra::Builtins {

NumberObject::NumberObject(double value)
    : Object(Runtime::ObjectType::Ordinary)
    , value_(value) {}

std::string NumberObject::toString(int radix) const {
    if (std::isnan(value_)) return "NaN";
    if (std::isinf(value_)) return value_ > 0 ? "Infinity" : "-Infinity";
    
    if (radix == 10) {
        std::ostringstream ss;
        ss << value_;
        return ss.str();
    }
    
    // Handle other radixes
    if (radix < 2 || radix > 36) radix = 10;
    
    int64_t intVal = static_cast<int64_t>(value_);
    std::string result;
    bool negative = intVal < 0;
    if (negative) intVal = -intVal;
    
    const char* digits = "0123456789abcdefghijklmnopqrstuvwxyz";
    do {
        result = digits[intVal % radix] + result;
        intVal /= radix;
    } while (intVal > 0);
    
    return negative ? "-" + result : result;
}

std::string NumberObject::toFixed(int digits) const {
    if (std::isnan(value_)) return "NaN";
    if (std::isinf(value_)) return value_ > 0 ? "Infinity" : "-Infinity";
    
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(digits) << value_;
    return ss.str();
}

std::string NumberObject::toExponential(int digits) const {
    if (std::isnan(value_)) return "NaN";
    if (std::isinf(value_)) return value_ > 0 ? "Infinity" : "-Infinity";
    
    std::ostringstream ss;
    ss << std::scientific << std::setprecision(digits) << value_;
    return ss.str();
}

std::string NumberObject::toPrecision(int precision) const {
    if (std::isnan(value_)) return "NaN";
    if (std::isinf(value_)) return value_ > 0 ? "Infinity" : "-Infinity";
    
    std::ostringstream ss;
    ss << std::setprecision(precision) << value_;
    return ss.str();
}

// =============================================================================
// NumberBuiltin Implementation
// =============================================================================

Value NumberBuiltin::constructor(Runtime::Context*, const std::vector<Value>& args) {
    double value = 0;
    if (!args.empty() && args[0].isNumber()) {
        value = args[0].asNumber();
    }
    return Value::object(new NumberObject(value));
}

Value NumberBuiltin::valueOf(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isObject()) return Value::number(0);
    NumberObject* n = dynamic_cast<NumberObject*>(args[0].asObject());
    return n ? Value::number(n->valueOf()) : Value::number(0);
}

Value NumberBuiltin::toString(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isObject()) return Value::string(new Runtime::String("0"));
    NumberObject* n = dynamic_cast<NumberObject*>(args[0].asObject());
    int radix = args.size() > 1 && args[1].isNumber() ? static_cast<int>(args[1].asNumber()) : 10;
    return n ? Value::string(new Runtime::String(n->toString(radix))) 
             : Value::string(new Runtime::String("0"));
}

Value NumberBuiltin::toFixed(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isObject()) return Value::string(new Runtime::String("0"));
    NumberObject* n = dynamic_cast<NumberObject*>(args[0].asObject());
    int digits = args.size() > 1 && args[1].isNumber() ? static_cast<int>(args[1].asNumber()) : 0;
    return n ? Value::string(new Runtime::String(n->toFixed(digits))) 
             : Value::string(new Runtime::String("0"));
}

Value NumberBuiltin::toExponential(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isObject()) return Value::string(new Runtime::String("0"));
    NumberObject* n = dynamic_cast<NumberObject*>(args[0].asObject());
    int digits = args.size() > 1 && args[1].isNumber() ? static_cast<int>(args[1].asNumber()) : 6;
    return n ? Value::string(new Runtime::String(n->toExponential(digits))) 
             : Value::string(new Runtime::String("0"));
}

Value NumberBuiltin::toPrecision(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isObject()) return Value::string(new Runtime::String("0"));
    NumberObject* n = dynamic_cast<NumberObject*>(args[0].asObject());
    int precision = args.size() > 1 && args[1].isNumber() ? static_cast<int>(args[1].asNumber()) : 6;
    return n ? Value::string(new Runtime::String(n->toPrecision(precision))) 
             : Value::string(new Runtime::String("0"));
}

Value NumberBuiltin::isNaN(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isNumber()) return Value::boolean(false);
    return Value::boolean(std::isnan(args[0].asNumber()));
}

Value NumberBuiltin::isFinite(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isNumber()) return Value::boolean(false);
    return Value::boolean(std::isfinite(args[0].asNumber()));
}

Value NumberBuiltin::isInteger(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isNumber()) return Value::boolean(false);
    double val = args[0].asNumber();
    return Value::boolean(std::isfinite(val) && val == std::trunc(val));
}

Value NumberBuiltin::isSafeInteger(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isNumber()) return Value::boolean(false);
    double val = args[0].asNumber();
    return Value::boolean(std::isfinite(val) && val == std::trunc(val) && 
                          std::abs(val) <= NumberObject::MAX_SAFE_INTEGER);
}

Value NumberBuiltin::parseFloat(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::number(std::numeric_limits<double>::quiet_NaN());
    std::string str = static_cast<Runtime::String*>(args[0].asObject())->value();
    return Value::number(std::strtod(str.c_str(), nullptr));
}

Value NumberBuiltin::parseInt(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::number(std::numeric_limits<double>::quiet_NaN());
    std::string str = static_cast<Runtime::String*>(args[0].asObject())->value();
    int radix = args.size() > 1 && args[1].isNumber() ? static_cast<int>(args[1].asNumber()) : 10;
    return Value::number(static_cast<double>(std::strtoll(str.c_str(), nullptr, radix)));
}

Object* NumberBuiltin::createNumberPrototype() {
    return new Object();
}

} // namespace Zepra::Builtins
