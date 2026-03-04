#pragma once

#include <functional>
#include <optional>
#include <vector>
#include <future>
#include <coroutine>

namespace Zepra::Runtime {

template<typename T>
class AsyncIterator;

template<typename T>
struct AsyncIteratorResult {
    T value;
    bool done;
};

template<typename T>
class AsyncIteratorHelpers {
public:
    using ValueType = T;
    using Mapper = std::function<std::future<T>(const T&)>;
    using Predicate = std::function<std::future<bool>(const T&)>;
    using Reducer = std::function<std::future<T>(const T&, const T&)>;
    using Consumer = std::function<std::future<void>(const T&)>;

    static AsyncIterator<T> map(AsyncIterator<T>& source, Mapper fn) {
        return AsyncIterator<T>([&source, fn]() -> std::future<AsyncIteratorResult<T>> {
            return std::async(std::launch::async, [&]() {
                auto result = source.next().get();
                if (result.done) return AsyncIteratorResult<T>{{}, true};
                return AsyncIteratorResult<T>{fn(result.value).get(), false};
            });
        });
    }

    static AsyncIterator<T> filter(AsyncIterator<T>& source, Predicate pred) {
        return AsyncIterator<T>([&source, pred]() -> std::future<AsyncIteratorResult<T>> {
            return std::async(std::launch::async, [&]() {
                while (true) {
                    auto result = source.next().get();
                    if (result.done) return AsyncIteratorResult<T>{{}, true};
                    if (pred(result.value).get()) {
                        return AsyncIteratorResult<T>{result.value, false};
                    }
                }
            });
        });
    }

    static AsyncIterator<T> take(AsyncIterator<T>& source, size_t limit) {
        size_t count = 0;
        return AsyncIterator<T>([&source, limit, count]() mutable -> std::future<AsyncIteratorResult<T>> {
            return std::async(std::launch::async, [&]() {
                if (count >= limit) return AsyncIteratorResult<T>{{}, true};
                auto result = source.next().get();
                if (result.done) return result;
                count++;
                return result;
            });
        });
    }

    static AsyncIterator<T> drop(AsyncIterator<T>& source, size_t count) {
        size_t dropped = 0;
        return AsyncIterator<T>([&source, count, dropped]() mutable -> std::future<AsyncIteratorResult<T>> {
            return std::async(std::launch::async, [&]() {
                while (dropped < count) {
                    auto result = source.next().get();
                    if (result.done) return result;
                    dropped++;
                }
                return source.next().get();
            });
        });
    }

    template<typename U>
    static AsyncIterator<U> flatMap(AsyncIterator<T>& source, 
                                     std::function<std::future<AsyncIterator<U>>(const T&)> fn) {
        std::optional<AsyncIterator<U>> innerIter;
        return AsyncIterator<U>([&source, fn, &innerIter]() -> std::future<AsyncIteratorResult<U>> {
            return std::async(std::launch::async, [&]() {
                while (true) {
                    if (innerIter.has_value()) {
                        auto innerResult = innerIter->next().get();
                        if (!innerResult.done) return innerResult;
                        innerIter.reset();
                    }
                    auto outerResult = source.next().get();
                    if (outerResult.done) return AsyncIteratorResult<U>{{}, true};
                    innerIter = fn(outerResult.value).get();
                }
            });
        });
    }

    static std::future<T> reduce(AsyncIterator<T>& source, Reducer fn, T initial) {
        return std::async(std::launch::async, [&source, fn, initial]() {
            T acc = initial;
            while (true) {
                auto result = source.next().get();
                if (result.done) break;
                acc = fn(acc, result.value).get();
            }
            return acc;
        });
    }

    static std::future<std::vector<T>> toArray(AsyncIterator<T>& source) {
        return std::async(std::launch::async, [&source]() {
            std::vector<T> result;
            while (true) {
                auto item = source.next().get();
                if (item.done) break;
                result.push_back(std::move(item.value));
            }
            return result;
        });
    }

    static std::future<void> forEach(AsyncIterator<T>& source, Consumer fn) {
        return std::async(std::launch::async, [&source, fn]() {
            while (true) {
                auto result = source.next().get();
                if (result.done) break;
                fn(result.value).get();
            }
        });
    }

    static std::future<bool> some(AsyncIterator<T>& source, Predicate pred) {
        return std::async(std::launch::async, [&source, pred]() {
            while (true) {
                auto result = source.next().get();
                if (result.done) return false;
                if (pred(result.value).get()) return true;
            }
        });
    }

    static std::future<bool> every(AsyncIterator<T>& source, Predicate pred) {
        return std::async(std::launch::async, [&source, pred]() {
            while (true) {
                auto result = source.next().get();
                if (result.done) return true;
                if (!pred(result.value).get()) return false;
            }
        });
    }

    static std::future<std::optional<T>> find(AsyncIterator<T>& source, Predicate pred) {
        return std::async(std::launch::async, [&source, pred]() -> std::optional<T> {
            while (true) {
                auto result = source.next().get();
                if (result.done) return std::nullopt;
                if (pred(result.value).get()) return result.value;
            }
        });
    }
};

template<typename T>
class AsyncIterator {
public:
    using NextFunction = std::function<std::future<AsyncIteratorResult<T>>()>;

private:
    NextFunction next_fn_;

public:
    explicit AsyncIterator(NextFunction fn) : next_fn_(std::move(fn)) {}

    std::future<AsyncIteratorResult<T>> next() {
        return next_fn_();
    }

    AsyncIterator<T> map(std::function<std::future<T>(const T&)> fn) {
        return AsyncIteratorHelpers<T>::map(*this, fn);
    }

    AsyncIterator<T> filter(std::function<std::future<bool>(const T&)> pred) {
        return AsyncIteratorHelpers<T>::filter(*this, pred);
    }

    AsyncIterator<T> take(size_t limit) {
        return AsyncIteratorHelpers<T>::take(*this, limit);
    }

    AsyncIterator<T> drop(size_t count) {
        return AsyncIteratorHelpers<T>::drop(*this, count);
    }

    std::future<T> reduce(std::function<std::future<T>(const T&, const T&)> fn, T initial) {
        return AsyncIteratorHelpers<T>::reduce(*this, fn, initial);
    }

    std::future<std::vector<T>> toArray() {
        return AsyncIteratorHelpers<T>::toArray(*this);
    }

    std::future<void> forEach(std::function<std::future<void>(const T&)> fn) {
        return AsyncIteratorHelpers<T>::forEach(*this, fn);
    }

    std::future<bool> some(std::function<std::future<bool>(const T&)> pred) {
        return AsyncIteratorHelpers<T>::some(*this, pred);
    }

    std::future<bool> every(std::function<std::future<bool>(const T&)> pred) {
        return AsyncIteratorHelpers<T>::every(*this, pred);
    }

    std::future<std::optional<T>> find(std::function<std::future<bool>(const T&)> pred) {
        return AsyncIteratorHelpers<T>::find(*this, pred);
    }

    template<typename Container>
    static AsyncIterator<T> from(const Container& container) {
        auto it = container.begin();
        auto end = container.end();
        return AsyncIterator<T>([it, end]() mutable -> std::future<AsyncIteratorResult<T>> {
            return std::async(std::launch::async, [&]() {
                if (it == end) return AsyncIteratorResult<T>{{}, true};
                T value = *it++;
                return AsyncIteratorResult<T>{std::move(value), false};
            });
        });
    }

    static AsyncIterator<T> fromAsync(std::function<std::future<std::optional<T>>()> producer) {
        return AsyncIterator<T>([producer]() -> std::future<AsyncIteratorResult<T>> {
            return std::async(std::launch::async, [producer]() {
                auto result = producer().get();
                if (!result.has_value()) return AsyncIteratorResult<T>{{}, true};
                return AsyncIteratorResult<T>{std::move(*result), false};
            });
        });
    }
};

}
