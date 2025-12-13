#pragma once

/**
 * @file iterator.hpp
 * @brief JavaScript Iterator protocol implementation
 */

#include "../config.hpp"
#include "../runtime/object.hpp"
#include "../runtime/value.hpp"
#include <functional>

namespace Zepra::Runtime { class Context; }

namespace Zepra::Runtime {

/**
 * @brief Iterator result { value, done }
 */
struct IteratorResult {
    Value value = Value::undefined();
    bool done = false;
};

/**
 * @brief Base Iterator object
 */
class Iterator : public Object {
public:
    Iterator() : Object(ObjectType::Ordinary) {}
    virtual ~Iterator() = default;
    
    /**
     * @brief Get next value from iterator
     */
    virtual IteratorResult next() = 0;
    
    /**
     * @brief Return from iterator (for-of break)
     */
    virtual IteratorResult return_(Value value);
    
    /**
     * @brief Throw into iterator
     */
    virtual IteratorResult throw_(Value exception);
};

/**
 * @brief Array Iterator
 */
class ArrayIterator : public Iterator {
public:
    ArrayIterator(Array* array, bool values = true);
    
    IteratorResult next() override;
    
private:
    Array* array_;
    size_t index_ = 0;
    bool returnValues_; // true = values, false = keys
};

/**
 * @brief String Iterator
 */
class StringIterator : public Iterator {
public:
    explicit StringIterator(String* str);
    
    IteratorResult next() override;
    
private:
    String* str_;
    size_t index_ = 0;
};

/**
 * @brief Object Entries Iterator
 */
class ObjectEntriesIterator : public Iterator {
public:
    ObjectEntriesIterator(Object* obj, bool keys, bool values);
    
    IteratorResult next() override;
    
private:
    Object* obj_;
    std::vector<std::string> keys_;
    size_t index_ = 0;
    bool returnKeys_;
    bool returnValues_;
};

/**
 * @brief Generator function result
 */
class Generator : public Iterator {
public:
    using GeneratorFunc = std::function<IteratorResult()>;
    
    explicit Generator(GeneratorFunc fn);
    
    IteratorResult next() override;
    
    bool isDone() const { return done_; }
    
private:
    GeneratorFunc fn_;
    bool done_ = false;
};

} // namespace Zepra::Runtime
