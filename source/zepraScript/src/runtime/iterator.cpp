/**
 * @file iterator.cpp
 * @brief JavaScript Iterator protocol implementation
 */

#include "zeprascript/runtime/iterator.hpp"

namespace Zepra::Runtime {

// =============================================================================
// Iterator Base
// =============================================================================

IteratorResult Iterator::return_(Value value) {
    return {value, true};
}

IteratorResult Iterator::throw_(Value) {
    return {Value::undefined(), true};
}

// =============================================================================
// ArrayIterator
// =============================================================================

ArrayIterator::ArrayIterator(Array* array, bool values)
    : array_(array), returnValues_(values) {}

IteratorResult ArrayIterator::next() {
    if (!array_ || index_ >= array_->length()) {
        return {Value::undefined(), true};
    }
    
    Value result;
    if (returnValues_) {
        result = array_->get(index_);
    } else {
        result = Value::number(static_cast<double>(index_));
    }
    
    index_++;
    return {result, false};
}

// =============================================================================
// StringIterator
// =============================================================================

StringIterator::StringIterator(String* str) : str_(str) {}

IteratorResult StringIterator::next() {
    if (!str_ || index_ >= str_->length()) {
        return {Value::undefined(), true};
    }
    
    // Get single character
    std::string ch = str_->value().substr(index_, 1);
    index_++;
    
    return {Value::string(new String(ch)), false};
}

// =============================================================================
// ObjectEntriesIterator
// =============================================================================

ObjectEntriesIterator::ObjectEntriesIterator(Object* obj, bool keys, bool values)
    : obj_(obj), returnKeys_(keys), returnValues_(values) {
    
    if (obj_) {
        keys_ = obj_->keys();
    }
}

IteratorResult ObjectEntriesIterator::next() {
    if (index_ >= keys_.size()) {
        return {Value::undefined(), true};
    }
    
    const std::string& key = keys_[index_++];
    
    if (returnKeys_ && returnValues_) {
        // Return [key, value] pair
        Array* pair = new Array();
        pair->push(Value::string(new String(key)));
        pair->push(obj_->get(key));
        return {Value::object(pair), false};
    } else if (returnKeys_) {
        return {Value::string(new String(key)), false};
    } else {
        return {obj_->get(key), false};
    }
}

// =============================================================================
// Generator
// =============================================================================

Generator::Generator(GeneratorFunc fn) : fn_(std::move(fn)) {}

IteratorResult Generator::next() {
    if (done_) {
        return {Value::undefined(), true};
    }
    
    IteratorResult result = fn_();
    if (result.done) {
        done_ = true;
    }
    return result;
}

} // namespace Zepra::Runtime
