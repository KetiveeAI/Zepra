/**
 * @file math.cpp
 * @brief JavaScript Math object implementation
 */

#include "zeprascript/builtins/math.hpp"
#include "zeprascript/runtime/function.hpp"

namespace Zepra::Builtins {

std::mt19937& MathBuiltin::getRandomEngine() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    return gen;
}

// Basic operations
Value MathBuiltin::abs(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isNumber()) return Value::number(std::numeric_limits<double>::quiet_NaN());
    return Value::number(std::abs(args[0].asNumber()));
}

Value MathBuiltin::ceil(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isNumber()) return Value::number(std::numeric_limits<double>::quiet_NaN());
    return Value::number(std::ceil(args[0].asNumber()));
}

Value MathBuiltin::floor(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isNumber()) return Value::number(std::numeric_limits<double>::quiet_NaN());
    return Value::number(std::floor(args[0].asNumber()));
}

Value MathBuiltin::round(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isNumber()) return Value::number(std::numeric_limits<double>::quiet_NaN());
    return Value::number(std::round(args[0].asNumber()));
}

Value MathBuiltin::trunc(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isNumber()) return Value::number(std::numeric_limits<double>::quiet_NaN());
    return Value::number(std::trunc(args[0].asNumber()));
}

Value MathBuiltin::sign(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isNumber()) return Value::number(std::numeric_limits<double>::quiet_NaN());
    double val = args[0].asNumber();
    if (val > 0) return Value::number(1);
    if (val < 0) return Value::number(-1);
    return Value::number(val); // +0, -0, or NaN
}

// Power/root
Value MathBuiltin::pow(Runtime::Context*, const std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isNumber() || !args[1].isNumber()) 
        return Value::number(std::numeric_limits<double>::quiet_NaN());
    return Value::number(std::pow(args[0].asNumber(), args[1].asNumber()));
}

Value MathBuiltin::sqrt(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isNumber()) return Value::number(std::numeric_limits<double>::quiet_NaN());
    return Value::number(std::sqrt(args[0].asNumber()));
}

Value MathBuiltin::cbrt(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isNumber()) return Value::number(std::numeric_limits<double>::quiet_NaN());
    return Value::number(std::cbrt(args[0].asNumber()));
}

Value MathBuiltin::hypot(Runtime::Context*, const std::vector<Value>& args) {
    double sum = 0;
    for (const auto& arg : args) {
        if (arg.isNumber()) {
            double val = arg.asNumber();
            sum += val * val;
        }
    }
    return Value::number(std::sqrt(sum));
}

// Exponential/logarithmic
Value MathBuiltin::exp(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isNumber()) return Value::number(std::numeric_limits<double>::quiet_NaN());
    return Value::number(std::exp(args[0].asNumber()));
}

Value MathBuiltin::expm1(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isNumber()) return Value::number(std::numeric_limits<double>::quiet_NaN());
    return Value::number(std::expm1(args[0].asNumber()));
}

Value MathBuiltin::log(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isNumber()) return Value::number(std::numeric_limits<double>::quiet_NaN());
    return Value::number(std::log(args[0].asNumber()));
}

Value MathBuiltin::log10(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isNumber()) return Value::number(std::numeric_limits<double>::quiet_NaN());
    return Value::number(std::log10(args[0].asNumber()));
}

Value MathBuiltin::log2(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isNumber()) return Value::number(std::numeric_limits<double>::quiet_NaN());
    return Value::number(std::log2(args[0].asNumber()));
}

Value MathBuiltin::log1p(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isNumber()) return Value::number(std::numeric_limits<double>::quiet_NaN());
    return Value::number(std::log1p(args[0].asNumber()));
}

// Trigonometric
Value MathBuiltin::sin(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isNumber()) return Value::number(std::numeric_limits<double>::quiet_NaN());
    return Value::number(std::sin(args[0].asNumber()));
}

Value MathBuiltin::cos(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isNumber()) return Value::number(std::numeric_limits<double>::quiet_NaN());
    return Value::number(std::cos(args[0].asNumber()));
}

Value MathBuiltin::tan(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isNumber()) return Value::number(std::numeric_limits<double>::quiet_NaN());
    return Value::number(std::tan(args[0].asNumber()));
}

Value MathBuiltin::asin(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isNumber()) return Value::number(std::numeric_limits<double>::quiet_NaN());
    return Value::number(std::asin(args[0].asNumber()));
}

Value MathBuiltin::acos(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isNumber()) return Value::number(std::numeric_limits<double>::quiet_NaN());
    return Value::number(std::acos(args[0].asNumber()));
}

Value MathBuiltin::atan(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isNumber()) return Value::number(std::numeric_limits<double>::quiet_NaN());
    return Value::number(std::atan(args[0].asNumber()));
}

Value MathBuiltin::atan2(Runtime::Context*, const std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isNumber() || !args[1].isNumber()) 
        return Value::number(std::numeric_limits<double>::quiet_NaN());
    return Value::number(std::atan2(args[0].asNumber(), args[1].asNumber()));
}

// Hyperbolic
Value MathBuiltin::sinh(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isNumber()) return Value::number(std::numeric_limits<double>::quiet_NaN());
    return Value::number(std::sinh(args[0].asNumber()));
}

Value MathBuiltin::cosh(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isNumber()) return Value::number(std::numeric_limits<double>::quiet_NaN());
    return Value::number(std::cosh(args[0].asNumber()));
}

Value MathBuiltin::tanh(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isNumber()) return Value::number(std::numeric_limits<double>::quiet_NaN());
    return Value::number(std::tanh(args[0].asNumber()));
}

Value MathBuiltin::asinh(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isNumber()) return Value::number(std::numeric_limits<double>::quiet_NaN());
    return Value::number(std::asinh(args[0].asNumber()));
}

Value MathBuiltin::acosh(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isNumber()) return Value::number(std::numeric_limits<double>::quiet_NaN());
    return Value::number(std::acosh(args[0].asNumber()));
}

Value MathBuiltin::atanh(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isNumber()) return Value::number(std::numeric_limits<double>::quiet_NaN());
    return Value::number(std::atanh(args[0].asNumber()));
}

// Min/max
Value MathBuiltin::min(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty()) return Value::number(std::numeric_limits<double>::infinity());
    double result = std::numeric_limits<double>::infinity();
    for (const auto& arg : args) {
        if (arg.isNumber()) {
            result = std::min(result, arg.asNumber());
        }
    }
    return Value::number(result);
}

Value MathBuiltin::max(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty()) return Value::number(-std::numeric_limits<double>::infinity());
    double result = -std::numeric_limits<double>::infinity();
    for (const auto& arg : args) {
        if (arg.isNumber()) {
            result = std::max(result, arg.asNumber());
        }
    }
    return Value::number(result);
}

// Random
Value MathBuiltin::random(Runtime::Context*, const std::vector<Value>&) {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    return Value::number(dist(getRandomEngine()));
}

// Bit manipulation
Value MathBuiltin::clz32(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isNumber()) return Value::number(32);
    uint32_t n = static_cast<uint32_t>(args[0].asNumber());
    if (n == 0) return Value::number(32);
    int count = 0;
    while ((n & 0x80000000) == 0) { n <<= 1; count++; }
    return Value::number(count);
}

Value MathBuiltin::imul(Runtime::Context*, const std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isNumber() || !args[1].isNumber()) return Value::number(0);
    int32_t a = static_cast<int32_t>(args[0].asNumber());
    int32_t b = static_cast<int32_t>(args[1].asNumber());
    return Value::number(static_cast<double>(a * b));
}

Value MathBuiltin::fround(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isNumber()) return Value::number(std::numeric_limits<double>::quiet_NaN());
    return Value::number(static_cast<float>(args[0].asNumber()));
}

Object* MathBuiltin::createMathObject() {
    Object* math = new Object();
    // TODO: Add all Math properties and methods
    return math;
}

} // namespace Zepra::Builtins
