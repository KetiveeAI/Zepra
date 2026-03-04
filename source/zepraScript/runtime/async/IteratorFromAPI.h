/**
 * @file IteratorFromAPI.h
 * @brief Iterator.from() and Iterator Wrappers
 */

#pragma once

#include <functional>
#include <optional>
#include <vector>

namespace Zepra::Runtime {

// =============================================================================
// Iterator Protocol
// =============================================================================

template<typename T>
struct IteratorResult {
    T value;
    bool done;
};

// =============================================================================
// Sync Iterator Wrapper
// =============================================================================

template<typename T>
class IteratorWrapper {
public:
    using NextFn = std::function<IteratorResult<T>()>;
    
    explicit IteratorWrapper(NextFn nextFn) : next_(std::move(nextFn)) {}
    
    IteratorResult<T> next() { return next_(); }
    
    IteratorWrapper& operator++() {
        lastResult_ = next_();
        return *this;
    }
    
    bool done() const { return lastResult_.done; }
    T value() const { return lastResult_.value; }

private:
    NextFn next_;
    IteratorResult<T> lastResult_ = {{}, true};
};

// =============================================================================
// Iterator.from()
// =============================================================================

template<typename Container>
class IteratorFrom {
public:
    using value_type = typename Container::value_type;
    
    explicit IteratorFrom(const Container& container)
        : container_(container), it_(container_.begin()) {}
    
    IteratorResult<value_type> next() {
        if (it_ == container_.end()) {
            return {{}, true};
        }
        value_type val = *it_;
        ++it_;
        return {val, false};
    }
    
    bool hasNext() const { return it_ != container_.end(); }

private:
    const Container& container_;
    typename Container::const_iterator it_;
};

template<typename Container>
IteratorFrom<Container> iteratorFrom(const Container& container) {
    return IteratorFrom<Container>(container);
}

// =============================================================================
// Range Iterator
// =============================================================================

class RangeIterator {
public:
    RangeIterator(int start, int end, int step = 1)
        : current_(start), end_(end), step_(step) {}
    
    IteratorResult<int> next() {
        if ((step_ > 0 && current_ >= end_) || (step_ < 0 && current_ <= end_)) {
            return {0, true};
        }
        int val = current_;
        current_ += step_;
        return {val, false};
    }
    
    bool hasNext() const {
        return (step_ > 0) ? current_ < end_ : current_ > end_;
    }

private:
    int current_, end_, step_;
};

inline RangeIterator range(int end) { return RangeIterator(0, end); }
inline RangeIterator range(int start, int end) { return RangeIterator(start, end); }
inline RangeIterator range(int start, int end, int step) { return RangeIterator(start, end, step); }

// =============================================================================
// Async Iterator Wrapper
// =============================================================================

template<typename T>
struct AsyncIteratorResult {
    std::optional<T> value;
    bool done;
};

template<typename T>
class AsyncIteratorWrapper {
public:
    using NextFn = std::function<AsyncIteratorResult<T>()>;
    
    explicit AsyncIteratorWrapper(NextFn nextFn) : next_(std::move(nextFn)) {}
    
    AsyncIteratorResult<T> next() { return next_(); }

private:
    NextFn next_;
};

// =============================================================================
// Collect to Vector
// =============================================================================

template<typename Iterator>
auto collectToVector(Iterator& it) {
    using ResultType = decltype(it.next());
    using ValueType = decltype(it.next().value);
    
    std::vector<ValueType> result;
    while (true) {
        auto item = it.next();
        if (item.done) break;
        result.push_back(item.value);
    }
    return result;
}

} // namespace Zepra::Runtime
