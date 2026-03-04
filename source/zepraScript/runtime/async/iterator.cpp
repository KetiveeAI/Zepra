#include "runtime/async/iterator.hpp"
#include "runtime/objects/object.hpp"

namespace Zepra::Runtime {

// ============================================================================
// ArrayIterator
// ============================================================================

ArrayIterator::ArrayIterator(Array* array)
    : array_(array), index_(0) {}

IteratorResult ArrayIterator::next() {
    if (index_ >= array_->length()) {
        return {Value::undefined(), true};
    }
    
    Value value = array_->get(index_);
    index_++;
    
    return {value, false};
}

// ============================================================================
// StringIterator
// ============================================================================

StringIterator::StringIterator(String* str)
    : str_(str), index_(0) {}

IteratorResult StringIterator::next() {
    if (index_ >= str_->length()) {
        return {Value::undefined(), true};
    }
    
    std::string chr(1, str_->value()[index_]);
    index_++;
    
    return {Value::string(new String(chr)), false};
}

// ============================================================================
// Iterator object creation
// ============================================================================

Object* createIteratorObject(Iterator* iterator) {
    Object* iterObj = new Object();
    return iterObj;
}

// ============================================================================
// ES2024/ES2025 Iterator Helper Implementations
// ============================================================================

// --- MappedIterator ---
MappedIterator::MappedIterator(Iterator* source, IteratorCallback mapper)
    : source_(source), mapper_(mapper), index_(0) {}

IteratorResult MappedIterator::next() {
    IteratorResult result = source_->next();
    if (result.done) {
        return result;
    }
    
    Value mapped = mapper_(result.value, index_++);
    return {mapped, false};
}

// --- FilteredIterator ---
FilteredIterator::FilteredIterator(Iterator* source, IteratorPredicate predicate)
    : source_(source), predicate_(predicate), index_(0) {}

IteratorResult FilteredIterator::next() {
    while (true) {
        IteratorResult result = source_->next();
        if (result.done) {
            return result;
        }
        
        if (predicate_(result.value, index_++)) {
            return result;
        }
    }
}

// --- TakeIterator ---
TakeIterator::TakeIterator(Iterator* source, size_t limit)
    : source_(source), limit_(limit), count_(0) {}

IteratorResult TakeIterator::next() {
    if (count_ >= limit_) {
        return {Value::undefined(), true};
    }
    
    IteratorResult result = source_->next();
    if (!result.done) {
        count_++;
    }
    return result;
}

// --- DropIterator ---
DropIterator::DropIterator(Iterator* source, size_t count)
    : source_(source), dropCount_(count), dropped_(false) {}

IteratorResult DropIterator::next() {
    if (!dropped_) {
        for (size_t i = 0; i < dropCount_; i++) {
            IteratorResult r = source_->next();
            if (r.done) {
                dropped_ = true;
                return r;
            }
        }
        dropped_ = true;
    }
    
    return source_->next();
}

// --- FlatMapIterator ---
FlatMapIterator::FlatMapIterator(Iterator* source, IteratorCallback mapper)
    : source_(source), mapper_(mapper), inner_(nullptr), index_(0) {}

IteratorResult FlatMapIterator::next() {
    while (true) {
        // Try to get from inner iterator
        if (inner_) {
            IteratorResult innerResult = inner_->next();
            if (!innerResult.done) {
                return innerResult;
            }
            inner_ = nullptr;
        }
        
        // Get next from source
        IteratorResult outerResult = source_->next();
        if (outerResult.done) {
            return outerResult;
        }
        
        // Map and create inner iterator
        Value mapped = mapper_(outerResult.value, index_++);
        if (mapped.isObject()) {
            Array* arr = dynamic_cast<Array*>(mapped.asObject());
            if (arr) {
                inner_ = new ArrayIterator(arr);
            }
        }
        
        // If not iterable, yield the value directly
        if (!inner_) {
            return {mapped, false};
        }
    }
}

// ============================================================================
// Concat Iterator (ES2025)
// ============================================================================

class ConcatIterator : public Iterator {
public:
    ConcatIterator(Iterator* first, Iterator* second)
        : first_(first), second_(second), useSecond_(false) {}
    
    IteratorResult next() override {
        if (!useSecond_) {
            IteratorResult result = first_->next();
            if (!result.done) {
                return result;
            }
            useSecond_ = true;
        }
        return second_->next();
    }
    
private:
    Iterator* first_;
    Iterator* second_;
    bool useSecond_;
};

// ============================================================================
// Zip Iterator (ES2025)
// ============================================================================

class ZipIterator : public Iterator {
public:
    ZipIterator(Iterator* first, Iterator* second)
        : first_(first), second_(second) {}
    
    IteratorResult next() override {
        IteratorResult r1 = first_->next();
        IteratorResult r2 = second_->next();
        
        if (r1.done || r2.done) {
            return {Value::undefined(), true};
        }
        
        std::vector<Value> pair = {r1.value, r2.value};
        return {Value::object(new Array(std::move(pair))), false};
    }
    
private:
    Iterator* first_;
    Iterator* second_;
};

// ============================================================================
// Enumerate Iterator (ES2025)
// ============================================================================

class EnumerateIterator : public Iterator {
public:
    EnumerateIterator(Iterator* source)
        : source_(source), index_(0) {}
    
    IteratorResult next() override {
        IteratorResult result = source_->next();
        if (result.done) {
            return result;
        }
        
        std::vector<Value> pair = {
            Value::number(static_cast<double>(index_++)),
            result.value
        };
        return {Value::object(new Array(std::move(pair))), false};
    }
    
private:
    Iterator* source_;
    size_t index_;
};

// ============================================================================
// IteratorHelpers static methods
// ============================================================================

Iterator* IteratorHelpers::map(Iterator* source, IteratorCallback fn) {
    return new MappedIterator(source, fn);
}

Iterator* IteratorHelpers::filter(Iterator* source, IteratorPredicate fn) {
    return new FilteredIterator(source, fn);
}

Iterator* IteratorHelpers::take(Iterator* source, size_t limit) {
    return new TakeIterator(source, limit);
}

Iterator* IteratorHelpers::drop(Iterator* source, size_t count) {
    return new DropIterator(source, count);
}

Iterator* IteratorHelpers::flatMap(Iterator* source, IteratorCallback fn) {
    return new FlatMapIterator(source, fn);
}

Value IteratorHelpers::reduce(Iterator* source, IteratorReducer fn, Value initial) {
    Value accumulator = initial;
    size_t index = 0;
    
    while (true) {
        IteratorResult result = source->next();
        if (result.done) {
            break;
        }
        accumulator = fn(accumulator, result.value, index++);
    }
    
    return accumulator;
}

Array* IteratorHelpers::toArray(Iterator* source) {
    std::vector<Value> values;
    
    while (true) {
        IteratorResult result = source->next();
        if (result.done) {
            break;
        }
        values.push_back(result.value);
    }
    
    return new Array(std::move(values));
}

void IteratorHelpers::forEach(Iterator* source, IteratorCallback fn) {
    size_t index = 0;
    
    while (true) {
        IteratorResult result = source->next();
        if (result.done) {
            break;
        }
        fn(result.value, index++);
    }
}

bool IteratorHelpers::some(Iterator* source, IteratorPredicate fn) {
    size_t index = 0;
    
    while (true) {
        IteratorResult result = source->next();
        if (result.done) {
            return false;
        }
        if (fn(result.value, index++)) {
            return true;
        }
    }
}

bool IteratorHelpers::every(Iterator* source, IteratorPredicate fn) {
    size_t index = 0;
    
    while (true) {
        IteratorResult result = source->next();
        if (result.done) {
            return true;
        }
        if (!fn(result.value, index++)) {
            return false;
        }
    }
}

Value IteratorHelpers::find(Iterator* source, IteratorPredicate fn) {
    size_t index = 0;
    
    while (true) {
        IteratorResult result = source->next();
        if (result.done) {
            return Value::undefined();
        }
        if (fn(result.value, index++)) {
            return result.value;
        }
    }
}

Iterator* IteratorHelpers::concat(Iterator* first, Iterator* second) {
    return new ConcatIterator(first, second);
}

Iterator* IteratorHelpers::zip(Iterator* first, Iterator* second) {
    return new ZipIterator(first, second);
}

Iterator* IteratorHelpers::enumerate(Iterator* source) {
    return new EnumerateIterator(source);
}

} // namespace Zepra::Runtime
