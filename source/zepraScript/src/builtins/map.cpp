/**
 * @file map.cpp
 * @brief JavaScript Map builtin implementation
 */

#include "zeprascript/builtins/map.hpp"
#include "zeprascript/runtime/function.hpp"

namespace Zepra::Builtins {

// =============================================================================
// MapObject Implementation
// =============================================================================

int MapObject::findKey(const Value& key) const {
    for (size_t i = 0; i < entries_.size(); ++i) {
        // Simple equality check (TODO: proper SameValueZero)
        if (entries_[i].first.equals(key)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void MapObject::set(const Value& key, const Value& value) {
    int idx = findKey(key);
    if (idx >= 0) {
        entries_[idx].second = value;
    } else {
        entries_.push_back({key, value});
    }
}

Value MapObject::get(const Value& key) const {
    int idx = findKey(key);
    if (idx >= 0) {
        return entries_[idx].second;
    }
    return Value::undefined();
}

bool MapObject::has(const Value& key) const {
    return findKey(key) >= 0;
}

bool MapObject::deleteKey(const Value& key) {
    int idx = findKey(key);
    if (idx >= 0) {
        entries_.erase(entries_.begin() + idx);
        return true;
    }
    return false;
}

void MapObject::clear() {
    entries_.clear();
}

std::vector<std::pair<Value, Value>> MapObject::entries() const {
    return entries_;
}

std::vector<Value> MapObject::keys() const {
    std::vector<Value> result;
    result.reserve(entries_.size());
    for (const auto& entry : entries_) {
        result.push_back(entry.first);
    }
    return result;
}

std::vector<Value> MapObject::values() const {
    std::vector<Value> result;
    result.reserve(entries_.size());
    for (const auto& entry : entries_) {
        result.push_back(entry.second);
    }
    return result;
}

// =============================================================================
// MapBuiltin Implementation
// =============================================================================

Value MapBuiltin::constructor(Runtime::Context*, const std::vector<Value>&) {
    return Value::object(new MapObject());
}

Value MapBuiltin::get(Runtime::Context*, const std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isObject()) return Value::undefined();
    
    MapObject* map = dynamic_cast<MapObject*>(args[0].asObject());
    if (!map) return Value::undefined();
    
    return map->get(args[1]);
}

Value MapBuiltin::set(Runtime::Context*, const std::vector<Value>& args) {
    if (args.size() < 3 || !args[0].isObject()) return Value::undefined();
    
    MapObject* map = dynamic_cast<MapObject*>(args[0].asObject());
    if (!map) return Value::undefined();
    
    map->set(args[1], args[2]);
    return args[0]; // Return map for chaining
}

Value MapBuiltin::has(Runtime::Context*, const std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isObject()) return Value::boolean(false);
    
    MapObject* map = dynamic_cast<MapObject*>(args[0].asObject());
    if (!map) return Value::boolean(false);
    
    return Value::boolean(map->has(args[1]));
}

Value MapBuiltin::deleteMethod(Runtime::Context*, const std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isObject()) return Value::boolean(false);
    
    MapObject* map = dynamic_cast<MapObject*>(args[0].asObject());
    if (!map) return Value::boolean(false);
    
    return Value::boolean(map->deleteKey(args[1]));
}

Value MapBuiltin::clear(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isObject()) return Value::undefined();
    
    MapObject* map = dynamic_cast<MapObject*>(args[0].asObject());
    if (map) map->clear();
    
    return Value::undefined();
}

Value MapBuiltin::size(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isObject()) return Value::number(0);
    
    MapObject* map = dynamic_cast<MapObject*>(args[0].asObject());
    if (!map) return Value::number(0);
    
    return Value::number(static_cast<double>(map->size()));
}

Object* MapBuiltin::createMapPrototype() {
    Object* proto = new Object();
    // Methods would be registered here via native functions
    return proto;
}

} // namespace Zepra::Builtins
