/**
 * @file bigint_object.hpp
 * @brief BigInt Object wrapper for GC integration
 *
 * Wraps the BigInt value type as a heap-allocated Object subclass
 * so BigInt values fit into the NaN-boxed Value system as objects.
 */

#pragma once

#include "runtime/objects/object.hpp"
#include "BigIntAPI.h"

namespace Zepra::Runtime {

/**
 * Heap-allocated BigInt wrapper.
 * Stored as Object with ObjectType::BigInt so Value can reference it
 * via the object pointer tag.
 */
class BigIntObject : public Object {
public:
    explicit BigIntObject(BigInt value)
        : Object(ObjectType::BigInt), value_(std::move(value)) {}

    explicit BigIntObject(int64_t value)
        : Object(ObjectType::BigInt), value_(value) {}

    explicit BigIntObject(const std::string& str, int radix = 10)
        : Object(ObjectType::BigInt), value_(str, radix) {}

    const BigInt& value() const { return value_; }
    BigInt& value() { return value_; }

    // Arithmetic — returns new heap-allocated BigIntObject
    BigIntObject* add(const BigIntObject* other) const {
        return new BigIntObject(value_ + other->value_);
    }

    BigIntObject* sub(const BigIntObject* other) const {
        return new BigIntObject(value_ - other->value_);
    }

    BigIntObject* mul(const BigIntObject* other) const {
        return new BigIntObject(value_ * other->value_);
    }

    BigIntObject* div(const BigIntObject* other) const {
        return new BigIntObject(value_ / other->value_);
    }

    BigIntObject* mod(const BigIntObject* other) const {
        return new BigIntObject(value_ % other->value_);
    }

    BigIntObject* bitwiseAnd(const BigIntObject* other) const {
        return new BigIntObject(value_ & other->value_);
    }

    BigIntObject* bitwiseOr(const BigIntObject* other) const {
        return new BigIntObject(value_ | other->value_);
    }

    BigIntObject* bitwiseXor(const BigIntObject* other) const {
        return new BigIntObject(value_ ^ other->value_);
    }

    BigIntObject* leftShift(int shift) const {
        return new BigIntObject(value_ << shift);
    }

    BigIntObject* rightShift(int shift) const {
        return new BigIntObject(value_ >> shift);
    }

    BigIntObject* negate() const {
        return new BigIntObject(value_.negate());
    }

    BigIntObject* bitwiseNot() const {
        return new BigIntObject(~value_);
    }

    BigIntObject* exponentiate(const BigIntObject* other) const {
        return new BigIntObject(value_.pow(other->value_));
    }

    // Comparison
    int compare(const BigIntObject* other) const {
        return value_.compare(other->value_);
    }

    // Conversion
    std::string toString(int radix = 10) const {
        return value_.toString(radix);
    }

    double toNumber() const {
        return value_.toNumber();
    }

    bool isZero() const { return value_.isZero(); }

private:
    BigInt value_;
};

} // namespace Zepra::Runtime
