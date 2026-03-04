#pragma once

/**
 * @file number.hpp
 * @brief JavaScript Number object
 */

#include "../config.hpp"
#include "runtime/objects/object.hpp"
#include "runtime/objects/value.hpp"
#include <cmath>
#include <limits>

namespace Zepra::Runtime { class Context; }

namespace Zepra::Builtins {

using Runtime::Value;
using Runtime::Object;

/**
 * @brief JavaScript Number object wrapper
 */
class NumberObject : public Object {
public:
    explicit NumberObject(double value);
    
    double valueOf() const { return value_; }
    std::string toString(int radix = 10) const;
    std::string toFixed(int digits) const;
    std::string toExponential(int digits) const;
    std::string toPrecision(int precision) const;
    
    // Static constants
    static constexpr double MAX_VALUE = std::numeric_limits<double>::max();
    static constexpr double MIN_VALUE = std::numeric_limits<double>::min();
    static constexpr double MAX_SAFE_INTEGER = 9007199254740991.0;
    static constexpr double MIN_SAFE_INTEGER = -9007199254740991.0;
    static constexpr double POSITIVE_INFINITY = std::numeric_limits<double>::infinity();
    static constexpr double NEGATIVE_INFINITY = -std::numeric_limits<double>::infinity();
    static constexpr double NaN = std::numeric_limits<double>::quiet_NaN();
    static constexpr double EPSILON = std::numeric_limits<double>::epsilon();
    
private:
    double value_;
};

/**
 * @brief Number builtin methods
 */
class NumberBuiltin {
public:
    static Value constructor(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value valueOf(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value toString(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value toFixed(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value toExponential(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value toPrecision(Runtime::Context* ctx, const std::vector<Value>& args);
    
    // Static methods
    static Value isNaN(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value isFinite(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value isInteger(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value isSafeInteger(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value parseFloat(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value parseInt(Runtime::Context* ctx, const std::vector<Value>& args);
    
    static Object* createNumberPrototype();
};

} // namespace Zepra::Builtins
