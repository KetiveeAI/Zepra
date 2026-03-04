#include "runtime/objects/set.hpp"

namespace Zepra::Runtime {

Set::Set() : Object() {}

void Set::add(const Value& value) {
    if (!has(value)) {
        values_.push_back(value);
    }
}

bool Set::has(const Value& value) const {
    return findIndex(value) != static_cast<size_t>(-1);
}

bool Set::deleteValue(const Value& value) {
    size_t index = findIndex(value);
    if (index != static_cast<size_t>(-1)) {
        values_.erase(values_.begin() + index);
        return true;
    }
    return false;
}

void Set::clear() {
    values_.clear();
}

void Set::forEach(std::function<void(const Value&)> callback) const {
    for (const auto& value : values_) {
        callback(value);
    }
}

size_t Set::findIndex(const Value& value) const {
    for (size_t i = 0; i < values_.size(); i++) {
        if (values_[i].strictEquals(value)) {
            return i;
        }
    }
    return static_cast<size_t>(-1);
}

} // namespace Zepra::Runtime
