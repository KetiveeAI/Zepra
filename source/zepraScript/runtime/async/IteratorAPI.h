/**
 * @file IteratorAPI.h
 * @brief Iterator Protocol Implementation
 * 
 * ECMAScript Iteration:
 * - IteratorResult: {value, done}
 * - Iterator protocol
 * - Built-in iterators (Array, Map, Set, String)
 */

#pragma once

#include <memory>
#include <optional>
#include <variant>
#include <vector>
#include <string>
#include <functional>
#include <stdexcept>

namespace Zepra::Runtime {

// =============================================================================
// Iterator Result
// =============================================================================

using IteratorValue = std::variant<
    std::monostate,
    bool,
    double,
    std::string,
    std::shared_ptr<void>
>;

/**
 * @brief {value, done} result from iterator.next()
 */
struct IteratorResult {
    IteratorValue value;
    bool done;
    
    static IteratorResult value(IteratorValue v) {
        return {std::move(v), false};
    }
    
    static IteratorResult done() {
        return {std::monostate{}, true};
    }
};

// =============================================================================
// Iterator Interface
// =============================================================================

/**
 * @brief Base iterator interface
 */
class Iterator {
public:
    virtual ~Iterator() = default;
    
    virtual IteratorResult next() = 0;
    
    virtual IteratorResult return_(IteratorValue value = std::monostate{}) {
        return IteratorResult::done();
    }
    
    virtual IteratorResult throw_(IteratorValue exception) {
        throw std::runtime_error("Iterator throw");
    }
};

// =============================================================================
// Iterable Interface
// =============================================================================

/**
 * @brief Interface for objects with Symbol.iterator
 */
class Iterable {
public:
    virtual ~Iterable() = default;
    virtual std::unique_ptr<Iterator> getIterator() = 0;
};

// =============================================================================
// Array Iterator
// =============================================================================

template<typename T>
class ArrayIterator : public Iterator {
public:
    explicit ArrayIterator(const std::vector<T>& array)
        : array_(array), index_(0) {}
    
    IteratorResult next() override {
        if (index_ >= array_.size()) {
            return IteratorResult::done();
        }
        
        return IteratorResult::value(getValue(array_[index_++]));
    }
    
private:
    IteratorValue getValue(const T& item) {
        if constexpr (std::is_same_v<T, double>) {
            return item;
        } else if constexpr (std::is_same_v<T, std::string>) {
            return item;
        } else if constexpr (std::is_same_v<T, bool>) {
            return item;
        } else {
            return std::monostate{};
        }
    }
    
    const std::vector<T>& array_;
    size_t index_;
};

// =============================================================================
// Array Entries/Keys/Values Iterator
// =============================================================================

enum class ArrayIterationType {
    Keys,
    Values,
    Entries
};

template<typename T>
class ArrayEntriesIterator : public Iterator {
public:
    ArrayEntriesIterator(const std::vector<T>& array, ArrayIterationType type)
        : array_(array), index_(0), type_(type) {}
    
    IteratorResult next() override {
        if (index_ >= array_.size()) {
            return IteratorResult::done();
        }
        
        size_t i = index_++;
        
        switch (type_) {
            case ArrayIterationType::Keys:
                return IteratorResult::value(static_cast<double>(i));
                
            case ArrayIterationType::Values:
                if constexpr (std::is_same_v<T, double>) {
                    return IteratorResult::value(array_[i]);
                } else if constexpr (std::is_same_v<T, std::string>) {
                    return IteratorResult::value(array_[i]);
                }
                return IteratorResult::value(std::monostate{});
                
            case ArrayIterationType::Entries:
                // Would return [index, value] pair
                return IteratorResult::value(std::monostate{});
        }
        
        return IteratorResult::done();
    }
    
private:
    const std::vector<T>& array_;
    size_t index_;
    ArrayIterationType type_;
};

// =============================================================================
// String Iterator
// =============================================================================

/**
 * @brief Iterates over string code points
 */
class StringIterator : public Iterator {
public:
    explicit StringIterator(const std::string& str)
        : str_(str), pos_(0) {}
    
    IteratorResult next() override {
        if (pos_ >= str_.size()) {
            return IteratorResult::done();
        }
        
        // Handle UTF-8 code points
        char first = str_[pos_];
        size_t charLen = 1;
        
        if ((first & 0x80) == 0) {
            charLen = 1;
        } else if ((first & 0xE0) == 0xC0) {
            charLen = 2;
        } else if ((first & 0xF0) == 0xE0) {
            charLen = 3;
        } else if ((first & 0xF8) == 0xF0) {
            charLen = 4;
        }
        
        std::string codePoint = str_.substr(pos_, charLen);
        pos_ += charLen;
        
        return IteratorResult::value(codePoint);
    }
    
private:
    std::string str_;
    size_t pos_;
};

// =============================================================================
// Map Iterator
// =============================================================================

template<typename K, typename V>
class MapIterator : public Iterator {
public:
    using Map = std::vector<std::pair<K, V>>;
    
    enum class Type { Keys, Values, Entries };
    
    MapIterator(const Map& map, Type type)
        : map_(map), type_(type), pos_(0) {}
    
    IteratorResult next() override {
        if (pos_ >= map_.size()) {
            return IteratorResult::done();
        }
        
        const auto& [key, value] = map_[pos_++];
        
        switch (type_) {
            case Type::Keys:
                return makeValue(key);
            case Type::Values:
                return makeValue(value);
            case Type::Entries:
                // Would return [key, value]
                return IteratorResult::value(std::monostate{});
        }
        
        return IteratorResult::done();
    }
    
private:
    template<typename T>
    IteratorResult makeValue(const T& item) {
        if constexpr (std::is_same_v<T, double>) {
            return IteratorResult::value(item);
        } else if constexpr (std::is_same_v<T, std::string>) {
            return IteratorResult::value(item);
        }
        return IteratorResult::value(std::monostate{});
    }
    
    const Map& map_;
    Type type_;
    size_t pos_;
};

// =============================================================================
// Set Iterator
// =============================================================================

template<typename T>
class SetIterator : public Iterator {
public:
    enum class Type { Values, Entries };
    
    SetIterator(const std::vector<T>& set, Type type)
        : set_(set), type_(type), pos_(0) {}
    
    IteratorResult next() override {
        if (pos_ >= set_.size()) {
            return IteratorResult::done();
        }
        
        const T& value = set_[pos_++];
        
        if constexpr (std::is_same_v<T, double>) {
            return IteratorResult::value(value);
        } else if constexpr (std::is_same_v<T, std::string>) {
            return IteratorResult::value(value);
        }
        
        return IteratorResult::value(std::monostate{});
    }
    
private:
    const std::vector<T>& set_;
    Type type_;
    size_t pos_;
};

// =============================================================================
// Iterator Helpers
// =============================================================================

/**
 * @brief Create iterator from function
 */
class FunctionIterator : public Iterator {
public:
    using NextFn = std::function<IteratorResult()>;
    
    explicit FunctionIterator(NextFn next) : nextFn_(std::move(next)) {}
    
    IteratorResult next() override {
        return nextFn_ ? nextFn_() : IteratorResult::done();
    }
    
private:
    NextFn nextFn_;
};

/**
 * @brief For-of iteration helper
 */
template<typename Fn>
void forOf(Iterator& iter, Fn callback) {
    while (true) {
        auto result = iter.next();
        if (result.done) break;
        callback(result.value);
    }
}

} // namespace Zepra::Runtime
