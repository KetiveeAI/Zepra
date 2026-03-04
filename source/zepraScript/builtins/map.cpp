/**
 * @file map.cpp
 * @brief JavaScript Map builtin implementation
 */

#include "builtins/map.hpp"
#include "runtime/objects/function.hpp"
#include <cmath>

namespace Zepra::Builtins {

// =============================================================================
// MapObject Implementation
// =============================================================================

int MapObject::findKey(const Value& key) const {
    for (size_t i = 0; i < entries_.size(); ++i) {
        const Value& k = entries_[i].first;
        // SameValueZero: NaN === NaN, +0 === -0, else strict equal
        if (key.isNumber() && k.isNumber()) {
            double a = key.asNumber();
            double b = k.asNumber();
            if (std::isnan(a) && std::isnan(b)) {
                return static_cast<int>(i);
            }
            if (a == b) {
                return static_cast<int>(i);
            }
        } else if (k.equals(key)) {
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

    proto->set("get", Value::object(
        new Runtime::Function("get", [](const Runtime::FunctionCallInfo& info) -> Value {
            if (!info.thisValue().isObject()) return Value::undefined();
            MapObject* map = dynamic_cast<MapObject*>(info.thisValue().asObject());
            if (!map || info.argumentCount() < 1) return Value::undefined();
            return map->get(info.argument(0));
        }, 1)));

    proto->set("set", Value::object(
        new Runtime::Function("set", [](const Runtime::FunctionCallInfo& info) -> Value {
            if (!info.thisValue().isObject()) return Value::undefined();
            MapObject* map = dynamic_cast<MapObject*>(info.thisValue().asObject());
            if (!map || info.argumentCount() < 2) return info.thisValue();
            map->set(info.argument(0), info.argument(1));
            return info.thisValue();
        }, 2)));

    proto->set("has", Value::object(
        new Runtime::Function("has", [](const Runtime::FunctionCallInfo& info) -> Value {
            if (!info.thisValue().isObject()) return Value::boolean(false);
            MapObject* map = dynamic_cast<MapObject*>(info.thisValue().asObject());
            if (!map || info.argumentCount() < 1) return Value::boolean(false);
            return Value::boolean(map->has(info.argument(0)));
        }, 1)));

    proto->set("delete", Value::object(
        new Runtime::Function("delete", [](const Runtime::FunctionCallInfo& info) -> Value {
            if (!info.thisValue().isObject()) return Value::boolean(false);
            MapObject* map = dynamic_cast<MapObject*>(info.thisValue().asObject());
            if (!map || info.argumentCount() < 1) return Value::boolean(false);
            return Value::boolean(map->deleteKey(info.argument(0)));
        }, 1)));

    proto->set("clear", Value::object(
        new Runtime::Function("clear", [](const Runtime::FunctionCallInfo& info) -> Value {
            if (!info.thisValue().isObject()) return Value::undefined();
            MapObject* map = dynamic_cast<MapObject*>(info.thisValue().asObject());
            if (map) map->clear();
            return Value::undefined();
        }, 0)));

    proto->set("forEach", Value::object(
        new Runtime::Function("forEach", [](const Runtime::FunctionCallInfo& info) -> Value {
            if (!info.thisValue().isObject() || info.argumentCount() < 1) return Value::undefined();
            MapObject* map = dynamic_cast<MapObject*>(info.thisValue().asObject());
            if (!map || !info.argument(0).isObject()) return Value::undefined();
            Runtime::Function* callback = dynamic_cast<Runtime::Function*>(info.argument(0).asObject());
            if (!callback) return Value::undefined();

            Value thisArg = info.argumentCount() > 1 ? info.argument(1) : Value::undefined();
            for (const auto& entry : map->entries()) {
                std::vector<Value> args = {entry.second, entry.first, info.thisValue()};
                callback->call(nullptr, thisArg, args);
            }
            return Value::undefined();
        }, 1)));

    return proto;
}

} // namespace Zepra::Builtins
