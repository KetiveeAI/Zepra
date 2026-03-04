/**
 * @file BigIntAPI.h
 * @brief BigInt Implementation
 * 
 * ECMAScript BigInt based on:
 * - JSC JSBigInt.h
 * - ECMA-262 7.2.13 BigInt
 * 
 * Arbitrary precision integer arithmetic
 */

#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <algorithm>
#include <stdexcept>
#include <cmath>
#include <sstream>
#include <iomanip>

namespace Zepra::Runtime {

// =============================================================================
// BigInt
// =============================================================================

class BigInt {
public:
    using Digit = uint32_t;
    using DoubleDigit = uint64_t;
    static constexpr int DigitBits = 32;
    static constexpr Digit DigitMax = 0xFFFFFFFF;
    
    BigInt() : negative_(false) { digits_.push_back(0); }
    
    explicit BigInt(int64_t value) {
        if (value < 0) {
            negative_ = true;
            value = -value;
        } else {
            negative_ = false;
        }
        
        if (value == 0) {
            digits_.push_back(0);
        } else {
            while (value > 0) {
                digits_.push_back(static_cast<Digit>(value & DigitMax));
                value >>= DigitBits;
            }
        }
    }
    
    explicit BigInt(const std::string& str, int radix = 10) {
        negative_ = false;
        digits_.push_back(0);
        
        size_t start = 0;
        if (!str.empty() && str[0] == '-') {
            negative_ = true;
            start = 1;
        } else if (!str.empty() && str[0] == '+') {
            start = 1;
        }
        
        for (size_t i = start; i < str.length(); ++i) {
            char c = str[i];
            int digit;
            
            if (c >= '0' && c <= '9') {
                digit = c - '0';
            } else if (c >= 'a' && c <= 'z') {
                digit = 10 + (c - 'a');
            } else if (c >= 'A' && c <= 'Z') {
                digit = 10 + (c - 'A');
            } else {
                continue;
            }
            
            if (digit >= radix) {
                throw std::invalid_argument("Invalid digit for radix");
            }
            
            multiplyByDigit(radix);
            addDigit(digit);
        }
        
        normalize();
    }
    
    bool isZero() const {
        return digits_.size() == 1 && digits_[0] == 0;
    }
    
    bool isNegative() const { return negative_ && !isZero(); }
    bool isPositive() const { return !negative_ && !isZero(); }
    
    BigInt negate() const {
        BigInt result = *this;
        if (!result.isZero()) {
            result.negative_ = !result.negative_;
        }
        return result;
    }
    
    BigInt abs() const {
        BigInt result = *this;
        result.negative_ = false;
        return result;
    }
    
    // Arithmetic operations
    BigInt operator+(const BigInt& other) const {
        if (negative_ == other.negative_) {
            BigInt result = addMagnitudes(*this, other);
            result.negative_ = negative_;
            return result;
        }
        
        int cmp = compareMagnitudes(*this, other);
        if (cmp == 0) return BigInt(0);
        
        if (cmp > 0) {
            BigInt result = subtractMagnitudes(*this, other);
            result.negative_ = negative_;
            return result;
        } else {
            BigInt result = subtractMagnitudes(other, *this);
            result.negative_ = other.negative_;
            return result;
        }
    }
    
    BigInt operator-(const BigInt& other) const {
        return *this + other.negate();
    }
    
    BigInt operator*(const BigInt& other) const {
        BigInt result = multiplyMagnitudes(*this, other);
        result.negative_ = negative_ != other.negative_;
        result.normalize();
        return result;
    }
    
    BigInt operator/(const BigInt& other) const {
        if (other.isZero()) {
            throw std::domain_error("Division by zero");
        }
        
        auto [quotient, remainder] = divmod(abs(), other.abs());
        quotient.negative_ = negative_ != other.negative_;
        quotient.normalize();
        return quotient;
    }
    
    BigInt operator%(const BigInt& other) const {
        if (other.isZero()) {
            throw std::domain_error("Division by zero");
        }
        
        auto [quotient, remainder] = divmod(abs(), other.abs());
        remainder.negative_ = negative_;
        remainder.normalize();
        return remainder;
    }
    
    // Bitwise operations
    BigInt operator&(const BigInt& other) const {
        return bitwiseOp(other, [](Digit a, Digit b) { return a & b; });
    }
    
    BigInt operator|(const BigInt& other) const {
        return bitwiseOp(other, [](Digit a, Digit b) { return a | b; });
    }
    
    BigInt operator^(const BigInt& other) const {
        return bitwiseOp(other, [](Digit a, Digit b) { return a ^ b; });
    }
    
    BigInt operator~() const {
        return ((*this) + BigInt(1)).negate();
    }
    
    BigInt operator<<(int shift) const {
        if (shift < 0) return *this >> (-shift);
        if (shift == 0 || isZero()) return *this;
        
        int digitShift = shift / DigitBits;
        int bitShift = shift % DigitBits;
        
        BigInt result;
        result.negative_ = negative_;
        result.digits_.resize(digits_.size() + digitShift + 1, 0);
        
        Digit carry = 0;
        for (size_t i = 0; i < digits_.size(); ++i) {
            DoubleDigit val = (static_cast<DoubleDigit>(digits_[i]) << bitShift) | carry;
            result.digits_[i + digitShift] = static_cast<Digit>(val);
            carry = static_cast<Digit>(val >> DigitBits);
        }
        
        if (carry) {
            result.digits_[digits_.size() + digitShift] = carry;
        }
        
        result.normalize();
        return result;
    }
    
    BigInt operator>>(int shift) const {
        if (shift < 0) return *this << (-shift);
        if (shift == 0 || isZero()) return *this;
        
        int digitShift = shift / DigitBits;
        int bitShift = shift % DigitBits;
        
        if (digitShift >= static_cast<int>(digits_.size())) {
            return negative_ ? BigInt(-1) : BigInt(0);
        }
        
        BigInt result;
        result.negative_ = negative_;
        result.digits_.resize(digits_.size() - digitShift, 0);
        
        for (size_t i = digitShift; i < digits_.size(); ++i) {
            result.digits_[i - digitShift] = digits_[i] >> bitShift;
            if (bitShift > 0 && i + 1 < digits_.size()) {
                result.digits_[i - digitShift] |= digits_[i + 1] << (DigitBits - bitShift);
            }
        }
        
        result.normalize();
        return result;
    }
    
    // Comparison
    int compare(const BigInt& other) const {
        if (negative_ != other.negative_) {
            return negative_ ? -1 : 1;
        }
        
        int cmp = compareMagnitudes(*this, other);
        return negative_ ? -cmp : cmp;
    }
    
    bool operator==(const BigInt& other) const { return compare(other) == 0; }
    bool operator!=(const BigInt& other) const { return compare(other) != 0; }
    bool operator<(const BigInt& other) const { return compare(other) < 0; }
    bool operator<=(const BigInt& other) const { return compare(other) <= 0; }
    bool operator>(const BigInt& other) const { return compare(other) > 0; }
    bool operator>=(const BigInt& other) const { return compare(other) >= 0; }
    
    // Exponentiation
    BigInt pow(const BigInt& exponent) const {
        if (exponent.isNegative()) {
            throw std::domain_error("Negative exponent");
        }
        
        if (exponent.isZero()) return BigInt(1);
        if (isZero()) return BigInt(0);
        
        BigInt base = *this;
        BigInt exp = exponent;
        BigInt result(1);
        
        while (!exp.isZero()) {
            if (exp.digits_[0] & 1) {
                result = result * base;
            }
            base = base * base;
            exp = exp >> 1;
        }
        
        return result;
    }
    
    // Conversion
    std::string toString(int radix = 10) const {
        if (radix < 2 || radix > 36) {
            throw std::invalid_argument("Radix must be 2-36");
        }
        
        if (isZero()) return "0";
        
        std::string result;
        BigInt temp = abs();
        
        while (!temp.isZero()) {
            auto [quotient, remainder] = divmod(temp, BigInt(radix));
            int digit = remainder.digits_[0];
            
            if (digit < 10) {
                result += ('0' + digit);
            } else {
                result += ('a' + digit - 10);
            }
            
            temp = quotient;
        }
        
        if (negative_) {
            result += '-';
        }
        
        std::reverse(result.begin(), result.end());
        return result;
    }
    
    double toNumber() const {
        if (isZero()) return 0.0;
        
        double result = 0.0;
        double multiplier = 1.0;
        
        for (size_t i = 0; i < digits_.size() && i < 3; ++i) {
            result += digits_[i] * multiplier;
            multiplier *= 4294967296.0;
        }
        
        return negative_ ? -result : result;
    }
    
    int64_t toInt64() const {
        if (isZero()) return 0;
        
        int64_t result = 0;
        if (digits_.size() >= 2) {
            result = (static_cast<int64_t>(digits_[1]) << 32) | digits_[0];
        } else {
            result = digits_[0];
        }
        
        return negative_ ? -result : result;
    }
    
    size_t bitLength() const {
        if (isZero()) return 0;
        
        size_t bits = (digits_.size() - 1) * DigitBits;
        Digit top = digits_.back();
        
        while (top) {
            ++bits;
            top >>= 1;
        }
        
        return bits;
    }
    
private:
    void normalize() {
        while (digits_.size() > 1 && digits_.back() == 0) {
            digits_.pop_back();
        }
        if (isZero()) {
            negative_ = false;
        }
    }
    
    void multiplyByDigit(Digit d) {
        DoubleDigit carry = 0;
        for (size_t i = 0; i < digits_.size(); ++i) {
            DoubleDigit prod = static_cast<DoubleDigit>(digits_[i]) * d + carry;
            digits_[i] = static_cast<Digit>(prod);
            carry = prod >> DigitBits;
        }
        if (carry) {
            digits_.push_back(static_cast<Digit>(carry));
        }
    }
    
    void addDigit(Digit d) {
        DoubleDigit carry = d;
        for (size_t i = 0; i < digits_.size() && carry; ++i) {
            DoubleDigit sum = static_cast<DoubleDigit>(digits_[i]) + carry;
            digits_[i] = static_cast<Digit>(sum);
            carry = sum >> DigitBits;
        }
        if (carry) {
            digits_.push_back(static_cast<Digit>(carry));
        }
    }
    
    static int compareMagnitudes(const BigInt& a, const BigInt& b) {
        if (a.digits_.size() != b.digits_.size()) {
            return a.digits_.size() > b.digits_.size() ? 1 : -1;
        }
        
        for (size_t i = a.digits_.size(); i-- > 0;) {
            if (a.digits_[i] != b.digits_[i]) {
                return a.digits_[i] > b.digits_[i] ? 1 : -1;
            }
        }
        
        return 0;
    }
    
    static BigInt addMagnitudes(const BigInt& a, const BigInt& b) {
        BigInt result;
        result.digits_.clear();
        
        size_t maxLen = std::max(a.digits_.size(), b.digits_.size());
        DoubleDigit carry = 0;
        
        for (size_t i = 0; i < maxLen || carry; ++i) {
            DoubleDigit sum = carry;
            if (i < a.digits_.size()) sum += a.digits_[i];
            if (i < b.digits_.size()) sum += b.digits_[i];
            
            result.digits_.push_back(static_cast<Digit>(sum));
            carry = sum >> DigitBits;
        }
        
        return result;
    }
    
    static BigInt subtractMagnitudes(const BigInt& a, const BigInt& b) {
        BigInt result;
        result.digits_.resize(a.digits_.size(), 0);
        
        int64_t borrow = 0;
        for (size_t i = 0; i < a.digits_.size(); ++i) {
            int64_t diff = static_cast<int64_t>(a.digits_[i]) - borrow;
            if (i < b.digits_.size()) diff -= b.digits_[i];
            
            if (diff < 0) {
                diff += static_cast<int64_t>(1) << DigitBits;
                borrow = 1;
            } else {
                borrow = 0;
            }
            
            result.digits_[i] = static_cast<Digit>(diff);
        }
        
        result.normalize();
        return result;
    }
    
    static BigInt multiplyMagnitudes(const BigInt& a, const BigInt& b) {
        BigInt result;
        result.digits_.resize(a.digits_.size() + b.digits_.size(), 0);
        
        for (size_t i = 0; i < a.digits_.size(); ++i) {
            DoubleDigit carry = 0;
            for (size_t j = 0; j < b.digits_.size(); ++j) {
                DoubleDigit prod = static_cast<DoubleDigit>(a.digits_[i]) * b.digits_[j];
                prod += result.digits_[i + j] + carry;
                result.digits_[i + j] = static_cast<Digit>(prod);
                carry = prod >> DigitBits;
            }
            result.digits_[i + b.digits_.size()] += static_cast<Digit>(carry);
        }
        
        result.normalize();
        return result;
    }
    
    static std::pair<BigInt, BigInt> divmod(const BigInt& dividend, const BigInt& divisor) {
        if (divisor.isZero()) {
            throw std::domain_error("Division by zero");
        }
        
        if (compareMagnitudes(dividend, divisor) < 0) {
            return {BigInt(0), dividend};
        }
        
        BigInt quotient;
        BigInt remainder = dividend;
        remainder.negative_ = false;
        
        BigInt d = divisor;
        d.negative_ = false;
        
        int shift = 0;
        while (compareMagnitudes(d << 1, remainder) <= 0) {
            ++shift;
        }
        
        while (shift >= 0) {
            BigInt shifted = d << shift;
            if (compareMagnitudes(remainder, shifted) >= 0) {
                remainder = remainder - shifted;
                quotient = quotient + (BigInt(1) << shift);
            }
            --shift;
        }
        
        quotient.normalize();
        remainder.normalize();
        return {quotient, remainder};
    }
    
    template<typename Op>
    BigInt bitwiseOp(const BigInt& other, Op op) const {
        size_t maxLen = std::max(digits_.size(), other.digits_.size());
        BigInt result;
        result.digits_.resize(maxLen, 0);
        
        for (size_t i = 0; i < maxLen; ++i) {
            Digit a = i < digits_.size() ? digits_[i] : 0;
            Digit b = i < other.digits_.size() ? other.digits_[i] : 0;
            result.digits_[i] = op(a, b);
        }
        
        result.normalize();
        return result;
    }
    
    bool negative_;
    std::vector<Digit> digits_;
};

// =============================================================================
// BigInt Factory
// =============================================================================

inline BigInt createBigInt(int64_t value) {
    return BigInt(value);
}

inline BigInt createBigInt(const std::string& str, int radix = 10) {
    return BigInt(str, radix);
}

} // namespace Zepra::Runtime
