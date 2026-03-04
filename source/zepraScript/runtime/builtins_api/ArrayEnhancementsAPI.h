/**
 * @file ArrayEnhancementsAPI.h
 * @brief Array Enhancements Implementation
 */

#pragma once

#include <vector>
#include <functional>
#include <set>
#include <map>
#include <algorithm>
#include <future>

namespace Zepra::Runtime {

// =============================================================================
// Array.fromAsync
// =============================================================================

template<typename T>
class ArrayFromAsync {
public:
    using AsyncIterator = std::function<std::future<std::pair<T, bool>>()>;
    
    static std::future<std::vector<T>> fromAsync(AsyncIterator iterator) {
        return std::async(std::launch::async, [iterator]() {
            std::vector<T> result;
            while (true) {
                auto future = iterator();
                auto [value, done] = future.get();
                if (done) break;
                result.push_back(value);
            }
            return result;
        });
    }
    
    template<typename U, typename F>
    static std::future<std::vector<U>> fromAsync(AsyncIterator iterator, F mapFn) {
        return std::async(std::launch::async, [iterator, mapFn]() {
            std::vector<U> result;
            size_t index = 0;
            while (true) {
                auto future = iterator();
                auto [value, done] = future.get();
                if (done) break;
                result.push_back(mapFn(value, index++));
            }
            return result;
        });
    }
};

// =============================================================================
// Array.prototype.uniqueBy
// =============================================================================

template<typename T, typename K>
std::vector<T> uniqueBy(const std::vector<T>& arr, std::function<K(const T&)> keyFn) {
    std::set<K> seen;
    std::vector<T> result;
    
    for (const auto& item : arr) {
        K key = keyFn(item);
        if (seen.find(key) == seen.end()) {
            seen.insert(key);
            result.push_back(item);
        }
    }
    
    return result;
}

template<typename T>
std::vector<T> unique(const std::vector<T>& arr) {
    return uniqueBy<T, T>(arr, [](const T& x) { return x; });
}

// =============================================================================
// Array.prototype.shuffle
// =============================================================================

template<typename T>
std::vector<T> shuffle(std::vector<T> arr) {
    for (size_t i = arr.size() - 1; i > 0; --i) {
        size_t j = std::rand() % (i + 1);
        std::swap(arr[i], arr[j]);
    }
    return arr;
}

// =============================================================================
// Array.prototype.sample
// =============================================================================

template<typename T>
std::vector<T> sample(const std::vector<T>& arr, size_t count) {
    if (count >= arr.size()) return arr;
    
    std::vector<T> shuffled = shuffle(arr);
    return std::vector<T>(shuffled.begin(), shuffled.begin() + count);
}

// =============================================================================
// Array.prototype.zip
// =============================================================================

template<typename T, typename U>
std::vector<std::pair<T, U>> zip(const std::vector<T>& a, const std::vector<U>& b) {
    std::vector<std::pair<T, U>> result;
    size_t len = std::min(a.size(), b.size());
    for (size_t i = 0; i < len; ++i) {
        result.emplace_back(a[i], b[i]);
    }
    return result;
}

// =============================================================================
// Array.prototype.compact
// =============================================================================

template<typename T>
std::vector<T> compact(const std::vector<std::optional<T>>& arr) {
    std::vector<T> result;
    for (const auto& item : arr) {
        if (item.has_value()) {
            result.push_back(*item);
        }
    }
    return result;
}

// =============================================================================
// Array.prototype.flatten
// =============================================================================

template<typename T>
std::vector<T> flatten(const std::vector<std::vector<T>>& arr) {
    std::vector<T> result;
    for (const auto& inner : arr) {
        result.insert(result.end(), inner.begin(), inner.end());
    }
    return result;
}

// =============================================================================
// Array.prototype.intersection/difference/symmetricDifference
// =============================================================================

template<typename T>
std::vector<T> intersection(const std::vector<T>& a, const std::vector<T>& b) {
    std::set<T> setB(b.begin(), b.end());
    std::vector<T> result;
    for (const auto& item : a) {
        if (setB.count(item)) result.push_back(item);
    }
    return result;
}

template<typename T>
std::vector<T> difference(const std::vector<T>& a, const std::vector<T>& b) {
    std::set<T> setB(b.begin(), b.end());
    std::vector<T> result;
    for (const auto& item : a) {
        if (!setB.count(item)) result.push_back(item);
    }
    return result;
}

template<typename T>
std::vector<T> symmetricDifference(const std::vector<T>& a, const std::vector<T>& b) {
    auto diffAB = difference(a, b);
    auto diffBA = difference(b, a);
    diffAB.insert(diffAB.end(), diffBA.begin(), diffBA.end());
    return diffAB;
}

} // namespace Zepra::Runtime
