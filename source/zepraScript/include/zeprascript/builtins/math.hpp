#pragma once

/**
 * @file math.hpp
 * @brief JavaScript Math object
 */

#include "../config.hpp"
#include "../runtime/object.hpp"
#include "../runtime/value.hpp"
#include <cmath>
#include <random>

namespace Zepra::Runtime { class Context; }

namespace Zepra::Builtins {

using Runtime::Value;
using Runtime::Object;

/**
 * @brief JavaScript Math object (static methods only)
 */
class MathBuiltin {
public:
    // Constants
    static constexpr double E = 2.718281828459045;
    static constexpr double LN10 = 2.302585092994046;
    static constexpr double LN2 = 0.6931471805599453;
    static constexpr double LOG10E = 0.4342944819032518;
    static constexpr double LOG2E = 1.4426950408889634;
    static constexpr double PI = 3.141592653589793;
    static constexpr double SQRT1_2 = 0.7071067811865476;
    static constexpr double SQRT2 = 1.4142135623730951;
    
    // Basic operations
    static Value abs(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value ceil(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value floor(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value round(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value trunc(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value sign(Runtime::Context* ctx, const std::vector<Value>& args);
    
    // Power/root
    static Value pow(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value sqrt(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value cbrt(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value hypot(Runtime::Context* ctx, const std::vector<Value>& args);
    
    // Exponential/logarithmic
    static Value exp(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value expm1(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value log(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value log10(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value log2(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value log1p(Runtime::Context* ctx, const std::vector<Value>& args);
    
    // Trigonometric
    static Value sin(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value cos(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value tan(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value asin(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value acos(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value atan(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value atan2(Runtime::Context* ctx, const std::vector<Value>& args);
    
    // Hyperbolic
    static Value sinh(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value cosh(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value tanh(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value asinh(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value acosh(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value atanh(Runtime::Context* ctx, const std::vector<Value>& args);
    
    // Min/max
    static Value min(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value max(Runtime::Context* ctx, const std::vector<Value>& args);
    
    // Random
    static Value random(Runtime::Context* ctx, const std::vector<Value>& args);
    
    // Bit manipulation
    static Value clz32(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value imul(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value fround(Runtime::Context* ctx, const std::vector<Value>& args);
    
    static Object* createMathObject();
    
private:
    static std::mt19937& getRandomEngine();
};

} // namespace Zepra::Builtins
