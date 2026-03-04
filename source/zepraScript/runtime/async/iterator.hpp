#pragma once

/**
 * @file iterator.hpp
 * @brief Iterator protocol implementation with ES2024/ES2025 helpers
 */

#include "config.hpp"
#include "runtime/objects/value.hpp"
#include "runtime/objects/object.hpp"
#include <functional>

namespace Zepra::Runtime {

class Function;

/**
 * @brief Iterator result object
 * {value: any, done: boolean}
 */
struct IteratorResult {
    Value value;
    bool done;
};

/**
 * @brief Iterator interface
 * Implements: {next() => {value, done}}
 */
class Iterator {
public:
    virtual ~Iterator() = default;
    virtual IteratorResult next() = 0;
};

/**
 * @brief Array iterator
 */
class ArrayIterator : public Iterator {
public:
    ArrayIterator(Array* array);
    IteratorResult next() override;
    
private:
    Array* array_;
    size_t index_;
};

/**
 * @brief String iterator (iterates over code points)
 */
class StringIterator : public Iterator {
public:
    StringIterator(String* str);
    IteratorResult next() override;
    
private:
    String* str_;
    size_t index_;
};

/**
 * @brief Create iterator object from Iterator instance
 */
Object* createIteratorObject(Iterator* iterator);

// =============================================================================
// ES2024/ES2025 Iterator Helpers
// =============================================================================

using IteratorCallback = std::function<Value(const Value&, size_t)>;
using IteratorPredicate = std::function<bool(const Value&, size_t)>;
using IteratorReducer = std::function<Value(const Value&, const Value&, size_t)>;

/**
 * @brief Mapped iterator - lazily applies function to each element
 */
class MappedIterator : public Iterator {
public:
    MappedIterator(Iterator* source, IteratorCallback mapper);
    IteratorResult next() override;
    
private:
    Iterator* source_;
    IteratorCallback mapper_;
    size_t index_;
};

/**
 * @brief Filtered iterator - lazily filters elements
 */
class FilteredIterator : public Iterator {
public:
    FilteredIterator(Iterator* source, IteratorPredicate predicate);
    IteratorResult next() override;
    
private:
    Iterator* source_;
    IteratorPredicate predicate_;
    size_t index_;
};

/**
 * @brief Take iterator - takes first n elements
 */
class TakeIterator : public Iterator {
public:
    TakeIterator(Iterator* source, size_t limit);
    IteratorResult next() override;
    
private:
    Iterator* source_;
    size_t limit_;
    size_t count_;
};

/**
 * @brief Drop iterator - skips first n elements
 */
class DropIterator : public Iterator {
public:
    DropIterator(Iterator* source, size_t count);
    IteratorResult next() override;
    
private:
    Iterator* source_;
    size_t dropCount_;
    bool dropped_;
};

/**
 * @brief FlatMap iterator - maps and flattens one level
 */
class FlatMapIterator : public Iterator {
public:
    FlatMapIterator(Iterator* source, IteratorCallback mapper);
    IteratorResult next() override;
    
private:
    Iterator* source_;
    IteratorCallback mapper_;
    Iterator* inner_;
    size_t index_;
};

/**
 * @brief Iterator helpers - static methods for ES2024/ES2025 iterator protocol
 */
class IteratorHelpers {
public:
    // Lazy transformations (return new iterators)
    static Iterator* map(Iterator* source, IteratorCallback fn);
    static Iterator* filter(Iterator* source, IteratorPredicate fn);
    static Iterator* take(Iterator* source, size_t limit);
    static Iterator* drop(Iterator* source, size_t count);
    static Iterator* flatMap(Iterator* source, IteratorCallback fn);
    
    // Consuming operations (iterate and return result)
    static Value reduce(Iterator* source, IteratorReducer fn, Value initial);
    static Array* toArray(Iterator* source);
    static void forEach(Iterator* source, IteratorCallback fn);
    static bool some(Iterator* source, IteratorPredicate fn);
    static bool every(Iterator* source, IteratorPredicate fn);
    static Value find(Iterator* source, IteratorPredicate fn);
    
    // ES2025 additions
    static Iterator* concat(Iterator* first, Iterator* second);
    static Iterator* zip(Iterator* first, Iterator* second);
    static Iterator* enumerate(Iterator* source);
};

} // namespace Zepra::Runtime

