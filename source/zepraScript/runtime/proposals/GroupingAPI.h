/**
 * @file GroupingAPI.h
 * @brief Object/Map Grouping Implementation
 */

#pragma once

#include <vector>
#include <map>
#include <unordered_map>
#include <functional>
#include <string>

namespace Zepra::Runtime {

// =============================================================================
// Object.groupBy
// =============================================================================

template<typename T, typename K>
std::map<K, std::vector<T>> objectGroupBy(
    const std::vector<T>& items,
    std::function<K(const T&)> keySelector) {
    
    std::map<K, std::vector<T>> result;
    for (const auto& item : items) {
        K key = keySelector(item);
        result[key].push_back(item);
    }
    return result;
}

template<typename T>
std::map<std::string, std::vector<T>> objectGroupBy(
    const std::vector<T>& items,
    std::function<std::string(const T&)> keySelector) {
    return objectGroupBy<T, std::string>(items, keySelector);
}

// =============================================================================
// Map.groupBy
// =============================================================================

template<typename T, typename K>
std::unordered_map<K, std::vector<T>> mapGroupBy(
    const std::vector<T>& items,
    std::function<K(const T&)> keySelector) {
    
    std::unordered_map<K, std::vector<T>> result;
    for (const auto& item : items) {
        K key = keySelector(item);
        result[key].push_back(item);
    }
    return result;
}

// =============================================================================
// Grouping Utilities
// =============================================================================

template<typename T, typename K, typename V>
std::map<K, V> groupByReduce(
    const std::vector<T>& items,
    std::function<K(const T&)> keySelector,
    std::function<V(V, const T&)> reducer,
    V initial) {
    
    std::map<K, V> result;
    for (const auto& item : items) {
        K key = keySelector(item);
        if (result.find(key) == result.end()) {
            result[key] = initial;
        }
        result[key] = reducer(result[key], item);
    }
    return result;
}

template<typename T, typename K>
std::map<K, size_t> countBy(
    const std::vector<T>& items,
    std::function<K(const T&)> keySelector) {
    
    return groupByReduce<T, K, size_t>(
        items,
        keySelector,
        [](size_t acc, const T&) { return acc + 1; },
        0
    );
}

template<typename T, typename K>
std::map<K, T> indexBy(
    const std::vector<T>& items,
    std::function<K(const T&)> keySelector) {
    
    std::map<K, T> result;
    for (const auto& item : items) {
        result[keySelector(item)] = item;
    }
    return result;
}

// =============================================================================
// Partition
// =============================================================================

template<typename T>
std::pair<std::vector<T>, std::vector<T>> partition(
    const std::vector<T>& items,
    std::function<bool(const T&)> predicate) {
    
    std::vector<T> truthy, falsy;
    for (const auto& item : items) {
        if (predicate(item)) {
            truthy.push_back(item);
        } else {
            falsy.push_back(item);
        }
    }
    return {truthy, falsy};
}

// =============================================================================
// Chunk
// =============================================================================

template<typename T>
std::vector<std::vector<T>> chunk(const std::vector<T>& items, size_t size) {
    std::vector<std::vector<T>> result;
    for (size_t i = 0; i < items.size(); i += size) {
        size_t end = std::min(i + size, items.size());
        result.push_back(std::vector<T>(items.begin() + i, items.begin() + end));
    }
    return result;
}

} // namespace Zepra::Runtime
