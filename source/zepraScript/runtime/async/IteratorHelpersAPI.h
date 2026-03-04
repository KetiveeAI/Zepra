/**
 * @file IteratorHelpersAPI.h
 * @brief Iterator Helpers Implementation
 */

#pragma once

#include <functional>
#include <optional>
#include <vector>
#include <iterator>

namespace Zepra::Runtime {

// =============================================================================
// Iterator Wrapper with Helpers
// =============================================================================

template<typename T, typename Iter>
class IteratorHelper {
public:
    using value_type = T;
    
    explicit IteratorHelper(Iter begin, Iter end) : current_(begin), end_(end) {}
    
    std::optional<T> next() {
        if (current_ == end_) return std::nullopt;
        return *current_++;
    }
    
    // map
    template<typename U>
    auto map(std::function<U(const T&)> fn) {
        std::vector<T> source(current_, end_);
        return MappedIterator<T, U>(source.begin(), source.end(), fn);
    }
    
    // filter
    auto filter(std::function<bool(const T&)> predicate) {
        std::vector<T> source(current_, end_);
        return FilteredIterator<T>(source.begin(), source.end(), predicate);
    }
    
    // take
    auto take(size_t limit) {
        std::vector<T> source(current_, end_);
        return TakeIterator<T>(source.begin(), source.end(), limit);
    }
    
    // drop
    auto drop(size_t count) {
        std::vector<T> source(current_, end_);
        return DropIterator<T>(source.begin(), source.end(), count);
    }
    
    // toArray
    std::vector<T> toArray() {
        return std::vector<T>(current_, end_);
    }
    
    // reduce
    template<typename U>
    U reduce(std::function<U(U, const T&)> fn, U initial) {
        U acc = initial;
        while (current_ != end_) {
            acc = fn(acc, *current_++);
        }
        return acc;
    }
    
    // forEach
    void forEach(std::function<void(const T&)> fn) {
        while (current_ != end_) {
            fn(*current_++);
        }
    }
    
    // some
    bool some(std::function<bool(const T&)> predicate) {
        while (current_ != end_) {
            if (predicate(*current_++)) return true;
        }
        return false;
    }
    
    // every
    bool every(std::function<bool(const T&)> predicate) {
        while (current_ != end_) {
            if (!predicate(*current_++)) return false;
        }
        return true;
    }
    
    // find
    std::optional<T> find(std::function<bool(const T&)> predicate) {
        while (current_ != end_) {
            T val = *current_++;
            if (predicate(val)) return val;
        }
        return std::nullopt;
    }

private:
    Iter current_;
    Iter end_;
};

// =============================================================================
// Mapped Iterator
// =============================================================================

template<typename T, typename U>
class MappedIterator {
public:
    using SourceIter = typename std::vector<T>::iterator;
    
    MappedIterator(SourceIter begin, SourceIter end, std::function<U(const T&)> fn)
        : current_(begin), end_(end), fn_(fn) {}
    
    std::optional<U> next() {
        if (current_ == end_) return std::nullopt;
        return fn_(*current_++);
    }
    
    std::vector<U> toArray() {
        std::vector<U> result;
        while (auto val = next()) {
            result.push_back(*val);
        }
        return result;
    }

private:
    SourceIter current_, end_;
    std::function<U(const T&)> fn_;
};

// =============================================================================
// Filtered Iterator
// =============================================================================

template<typename T>
class FilteredIterator {
public:
    using SourceIter = typename std::vector<T>::iterator;
    
    FilteredIterator(SourceIter begin, SourceIter end, std::function<bool(const T&)> predicate)
        : current_(begin), end_(end), predicate_(predicate) {}
    
    std::optional<T> next() {
        while (current_ != end_) {
            T val = *current_++;
            if (predicate_(val)) return val;
        }
        return std::nullopt;
    }
    
    std::vector<T> toArray() {
        std::vector<T> result;
        while (auto val = next()) {
            result.push_back(*val);
        }
        return result;
    }

private:
    SourceIter current_, end_;
    std::function<bool(const T&)> predicate_;
};

// =============================================================================
// Take Iterator
// =============================================================================

template<typename T>
class TakeIterator {
public:
    using SourceIter = typename std::vector<T>::iterator;
    
    TakeIterator(SourceIter begin, SourceIter end, size_t limit)
        : current_(begin), end_(end), remaining_(limit) {}
    
    std::optional<T> next() {
        if (remaining_ == 0 || current_ == end_) return std::nullopt;
        --remaining_;
        return *current_++;
    }
    
    std::vector<T> toArray() {
        std::vector<T> result;
        while (auto val = next()) {
            result.push_back(*val);
        }
        return result;
    }

private:
    SourceIter current_, end_;
    size_t remaining_;
};

// =============================================================================
// Drop Iterator
// =============================================================================

template<typename T>
class DropIterator {
public:
    using SourceIter = typename std::vector<T>::iterator;
    
    DropIterator(SourceIter begin, SourceIter end, size_t count)
        : current_(begin), end_(end), toSkip_(count) {}
    
    std::optional<T> next() {
        while (toSkip_ > 0 && current_ != end_) {
            ++current_;
            --toSkip_;
        }
        if (current_ == end_) return std::nullopt;
        return *current_++;
    }
    
    std::vector<T> toArray() {
        std::vector<T> result;
        while (auto val = next()) {
            result.push_back(*val);
        }
        return result;
    }

private:
    SourceIter current_, end_;
    size_t toSkip_;
};

// =============================================================================
// Factory
// =============================================================================

template<typename Container>
auto iteratorFrom(Container& c) {
    using T = typename Container::value_type;
    return IteratorHelper<T, typename Container::iterator>(c.begin(), c.end());
}

} // namespace Zepra::Runtime
