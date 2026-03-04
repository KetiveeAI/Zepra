#pragma once

#include <unordered_set>
#include <vector>
#include <algorithm>
#include <functional>

namespace Zepra::Runtime {

template<typename T, typename Hash = std::hash<T>, typename Equal = std::equal_to<T>>
class EnhancedSet {
private:
    std::unordered_set<T, Hash, Equal> data_;

public:
    EnhancedSet() = default;
    EnhancedSet(std::initializer_list<T> init) : data_(init) {}
    
    template<typename Container>
    explicit EnhancedSet(const Container& container) 
        : data_(container.begin(), container.end()) {}

    void add(const T& value) { data_.insert(value); }
    bool has(const T& value) const { return data_.find(value) != data_.end(); }
    bool remove(const T& value) { return data_.erase(value) > 0; }
    void clear() { data_.clear(); }
    size_t size() const { return data_.size(); }
    bool empty() const { return data_.empty(); }

    EnhancedSet<T, Hash, Equal> union_(const EnhancedSet<T, Hash, Equal>& other) const {
        EnhancedSet<T, Hash, Equal> result = *this;
        for (const auto& elem : other.data_) {
            result.add(elem);
        }
        return result;
    }

    EnhancedSet<T, Hash, Equal> intersection(const EnhancedSet<T, Hash, Equal>& other) const {
        EnhancedSet<T, Hash, Equal> result;
        for (const auto& elem : data_) {
            if (other.has(elem)) {
                result.add(elem);
            }
        }
        return result;
    }

    EnhancedSet<T, Hash, Equal> difference(const EnhancedSet<T, Hash, Equal>& other) const {
        EnhancedSet<T, Hash, Equal> result;
        for (const auto& elem : data_) {
            if (!other.has(elem)) {
                result.add(elem);
            }
        }
        return result;
    }

    EnhancedSet<T, Hash, Equal> symmetricDifference(const EnhancedSet<T, Hash, Equal>& other) const {
        EnhancedSet<T, Hash, Equal> result;
        for (const auto& elem : data_) {
            if (!other.has(elem)) {
                result.add(elem);
            }
        }
        for (const auto& elem : other.data_) {
            if (!has(elem)) {
                result.add(elem);
            }
        }
        return result;
    }

    bool isSubsetOf(const EnhancedSet<T, Hash, Equal>& other) const {
        for (const auto& elem : data_) {
            if (!other.has(elem)) return false;
        }
        return true;
    }

    bool isSupersetOf(const EnhancedSet<T, Hash, Equal>& other) const {
        return other.isSubsetOf(*this);
    }

    bool isDisjointFrom(const EnhancedSet<T, Hash, Equal>& other) const {
        for (const auto& elem : data_) {
            if (other.has(elem)) return false;
        }
        return true;
    }

    std::vector<T> toArray() const {
        return std::vector<T>(data_.begin(), data_.end());
    }

    void forEach(std::function<void(const T&)> callback) const {
        for (const auto& elem : data_) {
            callback(elem);
        }
    }

    auto begin() { return data_.begin(); }
    auto end() { return data_.end(); }
    auto begin() const { return data_.begin(); }
    auto end() const { return data_.end(); }

    EnhancedSet<T, Hash, Equal> filter(std::function<bool(const T&)> predicate) const {
        EnhancedSet<T, Hash, Equal> result;
        for (const auto& elem : data_) {
            if (predicate(elem)) {
                result.add(elem);
            }
        }
        return result;
    }

    template<typename U>
    EnhancedSet<U> map(std::function<U(const T&)> mapper) const {
        EnhancedSet<U> result;
        for (const auto& elem : data_) {
            result.add(mapper(elem));
        }
        return result;
    }

    bool some(std::function<bool(const T&)> predicate) const {
        for (const auto& elem : data_) {
            if (predicate(elem)) return true;
        }
        return false;
    }

    bool every(std::function<bool(const T&)> predicate) const {
        for (const auto& elem : data_) {
            if (!predicate(elem)) return false;
        }
        return true;
    }

    std::optional<T> find(std::function<bool(const T&)> predicate) const {
        for (const auto& elem : data_) {
            if (predicate(elem)) return elem;
        }
        return std::nullopt;
    }
};

class SetMethods {
public:
    template<typename T>
    static EnhancedSet<T> union_(const EnhancedSet<T>& a, const EnhancedSet<T>& b) {
        return a.union_(b);
    }

    template<typename T>
    static EnhancedSet<T> intersection(const EnhancedSet<T>& a, const EnhancedSet<T>& b) {
        return a.intersection(b);
    }

    template<typename T>
    static EnhancedSet<T> difference(const EnhancedSet<T>& a, const EnhancedSet<T>& b) {
        return a.difference(b);
    }

    template<typename T>
    static EnhancedSet<T> symmetricDifference(const EnhancedSet<T>& a, const EnhancedSet<T>& b) {
        return a.symmetricDifference(b);
    }

    template<typename T>
    static bool isSubsetOf(const EnhancedSet<T>& a, const EnhancedSet<T>& b) {
        return a.isSubsetOf(b);
    }

    template<typename T>
    static bool isSupersetOf(const EnhancedSet<T>& a, const EnhancedSet<T>& b) {
        return a.isSupersetOf(b);
    }

    template<typename T>
    static bool isDisjointFrom(const EnhancedSet<T>& a, const EnhancedSet<T>& b) {
        return a.isDisjointFrom(b);
    }
};

template<typename K, typename V, 
         typename Hash = std::hash<K>, 
         typename Equal = std::equal_to<K>>
class EnhancedMap {
private:
    std::unordered_map<K, V, Hash, Equal> data_;

public:
    EnhancedMap() = default;

    void set(const K& key, const V& value) { data_[key] = value; }
    std::optional<V> get(const K& key) const {
        auto it = data_.find(key);
        return it != data_.end() ? std::optional<V>(it->second) : std::nullopt;
    }
    bool has(const K& key) const { return data_.find(key) != data_.end(); }
    bool remove(const K& key) { return data_.erase(key) > 0; }
    void clear() { data_.clear(); }
    size_t size() const { return data_.size(); }
    bool empty() const { return data_.empty(); }

    std::vector<K> keys() const {
        std::vector<K> result;
        for (const auto& [k, v] : data_) result.push_back(k);
        return result;
    }

    std::vector<V> values() const {
        std::vector<V> result;
        for (const auto& [k, v] : data_) result.push_back(v);
        return result;
    }

    std::vector<std::pair<K, V>> entries() const {
        return std::vector<std::pair<K, V>>(data_.begin(), data_.end());
    }

    void forEach(std::function<void(const V&, const K&)> callback) const {
        for (const auto& [k, v] : data_) {
            callback(v, k);
        }
    }

    auto begin() { return data_.begin(); }
    auto end() { return data_.end(); }
    auto begin() const { return data_.begin(); }
    auto end() const { return data_.end(); }
};

}
