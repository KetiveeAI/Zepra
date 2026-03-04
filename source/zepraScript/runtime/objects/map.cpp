#include "runtime/objects/map.hpp"

namespace Zepra::Runtime {

Map::Map() : Object() {}

void Map::set(const Value& key, const Value& value) {
    size_t index = findIndex(key);
    if (index != static_cast<size_t>(-1)) {
        entries_[index].value = value;
    } else {
        entries_.push_back({key, value});
    }
}

Value Map::get(const Value& key) const {
    size_t index = findIndex(key);
    if (index != static_cast<size_t>(-1)) {
        return entries_[index].value;
    }
    return Value::undefined();
}

bool Map::has(const Value& key) const {
    return findIndex(key) != static_cast<size_t>(-1);
}

bool Map::deleteKey(const Value& key) {
    size_t index = findIndex(key);
    if (index != static_cast<size_t>(-1)) {
        entries_.erase(entries_.begin() + index);
        return true;
    }
    return false;
}

void Map::clear() {
    entries_.clear();
}

std::vector<Value> Map::keys() const {
    std::vector<Value> result;
    for (const auto& entry : entries_) {
        result.push_back(entry.key);
    }
    return result;
}

std::vector<Value> Map::values() const {
    std::vector<Value> result;
    for (const auto& entry : entries_) {
        result.push_back(entry.value);
    }
    return result;
}

std::vector<std::pair<Value, Value>> Map::entries() const {
    std::vector<std::pair<Value, Value>> result;
    for (const auto& entry : entries_) {
        result.push_back({entry.key, entry.value});
    }
    return result;
}

void Map::forEach(std::function<void(const Value&, const Value&)> callback) const {
    for (const auto& entry : entries_) {
        callback(entry.value, entry.key);
    }
}

size_t Map::findIndex(const Value& key) const {
    for (size_t i = 0; i < entries_.size(); i++) {
        if (entries_[i].key.strictEquals(key)) {
            return i;
        }
    }
    return static_cast<size_t>(-1);
}

} // namespace Zepra::Runtime
