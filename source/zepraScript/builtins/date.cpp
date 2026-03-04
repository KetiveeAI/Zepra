/**
 * @file date.cpp
 * @brief JavaScript Date object implementation
 */

#include "builtins/date.hpp"
#include "runtime/objects/function.hpp"
#include <iomanip>
#include <sstream>

namespace Zepra::Builtins {

// =============================================================================
// DateObject Implementation
// =============================================================================

DateObject::DateObject()
    : Object(Runtime::ObjectType::Ordinary) {
    auto now = std::chrono::system_clock::now();
    timestamp_ = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
}

DateObject::DateObject(double timestamp)
    : Object(Runtime::ObjectType::Ordinary)
    , timestamp_(timestamp) {}

DateObject::DateObject(int year, int month, int day, int hours, 
                       int minutes, int seconds, int ms)
    : Object(Runtime::ObjectType::Ordinary) {
    std::tm tm = {};
    tm.tm_year = year - 1900;
    tm.tm_mon = month;
    tm.tm_mday = day;
    tm.tm_hour = hours;
    tm.tm_min = minutes;
    tm.tm_sec = seconds;
    fromTm(tm);
    timestamp_ += ms;
}

std::tm DateObject::toTm() const {
    std::time_t t = static_cast<std::time_t>(timestamp_ / 1000);
    std::tm tm;
    localtime_r(&t, &tm);
    return tm;
}

void DateObject::fromTm(const std::tm& tm) {
    std::tm copy = tm;
    timestamp_ = static_cast<double>(mktime(&copy)) * 1000;
}

int DateObject::getFullYear() const { return toTm().tm_year + 1900; }
int DateObject::getMonth() const { return toTm().tm_mon; }
int DateObject::getDate() const { return toTm().tm_mday; }
int DateObject::getDay() const { return toTm().tm_wday; }
int DateObject::getHours() const { return toTm().tm_hour; }
int DateObject::getMinutes() const { return toTm().tm_min; }
int DateObject::getSeconds() const { return toTm().tm_sec; }
int DateObject::getMilliseconds() const { return static_cast<int>(timestamp_) % 1000; }

int DateObject::getUTCFullYear() const {
    std::time_t t = static_cast<std::time_t>(timestamp_ / 1000);
    std::tm tm;
    gmtime_r(&t, &tm);
    return tm.tm_year + 1900;
}

int DateObject::getUTCMonth() const {
    std::time_t t = static_cast<std::time_t>(timestamp_ / 1000);
    std::tm tm;
    gmtime_r(&t, &tm);
    return tm.tm_mon;
}

int DateObject::getUTCDate() const {
    std::time_t t = static_cast<std::time_t>(timestamp_ / 1000);
    std::tm tm;
    gmtime_r(&t, &tm);
    return tm.tm_mday;
}

int DateObject::getUTCDay() const {
    std::time_t t = static_cast<std::time_t>(timestamp_ / 1000);
    std::tm tm;
    gmtime_r(&t, &tm);
    return tm.tm_wday;
}

int DateObject::getUTCHours() const {
    std::time_t t = static_cast<std::time_t>(timestamp_ / 1000);
    std::tm tm;
    gmtime_r(&t, &tm);
    return tm.tm_hour;
}

int DateObject::getUTCMinutes() const {
    std::time_t t = static_cast<std::time_t>(timestamp_ / 1000);
    std::tm tm;
    gmtime_r(&t, &tm);
    return tm.tm_min;
}

int DateObject::getUTCSeconds() const {
    std::time_t t = static_cast<std::time_t>(timestamp_ / 1000);
    std::tm tm;
    gmtime_r(&t, &tm);
    return tm.tm_sec;
}

int DateObject::getUTCMilliseconds() const {
    return getMilliseconds();
}

void DateObject::setFullYear(int year) {
    std::tm tm = toTm();
    tm.tm_year = year - 1900;
    fromTm(tm);
}

void DateObject::setMonth(int month) {
    std::tm tm = toTm();
    tm.tm_mon = month;
    fromTm(tm);
}

void DateObject::setDate(int day) {
    std::tm tm = toTm();
    tm.tm_mday = day;
    fromTm(tm);
}

void DateObject::setHours(int hours) {
    std::tm tm = toTm();
    tm.tm_hour = hours;
    fromTm(tm);
}

void DateObject::setMinutes(int minutes) {
    std::tm tm = toTm();
    tm.tm_min = minutes;
    fromTm(tm);
}

void DateObject::setSeconds(int seconds) {
    std::tm tm = toTm();
    tm.tm_sec = seconds;
    fromTm(tm);
}

void DateObject::setMilliseconds(int ms) {
    timestamp_ = std::floor(timestamp_ / 1000) * 1000 + ms;
}

std::string DateObject::toString() const {
    char buf[64];
    std::tm tm = toTm();
    strftime(buf, sizeof(buf), "%a %b %d %Y %H:%M:%S GMT%z", &tm);
    return buf;
}

std::string DateObject::toDateString() const {
    char buf[32];
    std::tm tm = toTm();
    strftime(buf, sizeof(buf), "%a %b %d %Y", &tm);
    return buf;
}

std::string DateObject::toTimeString() const {
    char buf[32];
    std::tm tm = toTm();
    strftime(buf, sizeof(buf), "%H:%M:%S GMT%z", &tm);
    return buf;
}

std::string DateObject::toISOString() const {
    std::ostringstream ss;
    std::tm tm = toTm();
    ss << std::setfill('0') << std::setw(4) << (tm.tm_year + 1900) << "-"
       << std::setw(2) << (tm.tm_mon + 1) << "-"
       << std::setw(2) << tm.tm_mday << "T"
       << std::setw(2) << tm.tm_hour << ":"
       << std::setw(2) << tm.tm_min << ":"
       << std::setw(2) << tm.tm_sec << "."
       << std::setw(3) << getMilliseconds() << "Z";
    return ss.str();
}

std::string DateObject::toUTCString() const {
    char buf[64];
    std::time_t t = static_cast<std::time_t>(timestamp_ / 1000);
    std::tm tm;
    gmtime_r(&t, &tm);
    strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", &tm);
    return buf;
}

std::string DateObject::toLocaleString() const {
    return toString();
}

int DateObject::getTimezoneOffset() const {
    std::tm tm = toTm();
    return -static_cast<int>(tm.tm_gmtoff / 60);
}

// =============================================================================
// DateBuiltin Implementation
// =============================================================================

Value DateBuiltin::constructor(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty()) {
        return Value::object(new DateObject());
    }
    if (args.size() == 1 && args[0].isNumber()) {
        return Value::object(new DateObject(args[0].asNumber()));
    }
    int year = args.size() > 0 && args[0].isNumber() ? static_cast<int>(args[0].asNumber()) : 1970;
    int month = args.size() > 1 && args[1].isNumber() ? static_cast<int>(args[1].asNumber()) : 0;
    int day = args.size() > 2 && args[2].isNumber() ? static_cast<int>(args[2].asNumber()) : 1;
    int hours = args.size() > 3 && args[3].isNumber() ? static_cast<int>(args[3].asNumber()) : 0;
    int mins = args.size() > 4 && args[4].isNumber() ? static_cast<int>(args[4].asNumber()) : 0;
    int secs = args.size() > 5 && args[5].isNumber() ? static_cast<int>(args[5].asNumber()) : 0;
    int ms = args.size() > 6 && args[6].isNumber() ? static_cast<int>(args[6].asNumber()) : 0;
    
    return Value::object(new DateObject(year, month, day, hours, mins, secs, ms));
}

Value DateBuiltin::now(Runtime::Context*, const std::vector<Value>&) {
    auto now = std::chrono::system_clock::now();
    return Value::number(std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count());
}

Value DateBuiltin::parse(Runtime::Context*, const std::vector<Value>&) {
    return Value::number(std::numeric_limits<double>::quiet_NaN());
}

Value DateBuiltin::UTC(Runtime::Context*, const std::vector<Value>&) {
    return Value::number(0);
}

Value DateBuiltin::getTime(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isObject()) return Value::undefined();
    DateObject* d = dynamic_cast<DateObject*>(args[0].asObject());
    return d ? Value::number(d->getTime()) : Value::undefined();
}

Value DateBuiltin::getFullYear(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isObject()) return Value::undefined();
    DateObject* d = dynamic_cast<DateObject*>(args[0].asObject());
    return d ? Value::number(d->getFullYear()) : Value::undefined();
}

Value DateBuiltin::getMonth(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isObject()) return Value::undefined();
    DateObject* d = dynamic_cast<DateObject*>(args[0].asObject());
    return d ? Value::number(d->getMonth()) : Value::undefined();
}

Value DateBuiltin::getDate(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isObject()) return Value::undefined();
    DateObject* d = dynamic_cast<DateObject*>(args[0].asObject());
    return d ? Value::number(d->getDate()) : Value::undefined();
}

Value DateBuiltin::getDay(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isObject()) return Value::undefined();
    DateObject* d = dynamic_cast<DateObject*>(args[0].asObject());
    return d ? Value::number(d->getDay()) : Value::undefined();
}

Value DateBuiltin::getHours(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isObject()) return Value::undefined();
    DateObject* d = dynamic_cast<DateObject*>(args[0].asObject());
    return d ? Value::number(d->getHours()) : Value::undefined();
}

Value DateBuiltin::getMinutes(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isObject()) return Value::undefined();
    DateObject* d = dynamic_cast<DateObject*>(args[0].asObject());
    return d ? Value::number(d->getMinutes()) : Value::undefined();
}

Value DateBuiltin::getSeconds(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isObject()) return Value::undefined();
    DateObject* d = dynamic_cast<DateObject*>(args[0].asObject());
    return d ? Value::number(d->getSeconds()) : Value::undefined();
}

Value DateBuiltin::toString(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isObject()) return Value::undefined();
    DateObject* d = dynamic_cast<DateObject*>(args[0].asObject());
    return d ? Value::string(new Runtime::String(d->toString())) : Value::undefined();
}

Value DateBuiltin::toISOString(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isObject()) return Value::undefined();
    DateObject* d = dynamic_cast<DateObject*>(args[0].asObject());
    return d ? Value::string(new Runtime::String(d->toISOString())) : Value::undefined();
}

Object* DateBuiltin::createDatePrototype() {
    Object* proto = new Object();

    // Helper macro equivalent — each method extracts `this` as DateObject
    auto dateGetter = [](const char* name, auto getter) {
        return Value::object(new Runtime::Function(name,
            [getter](const Runtime::FunctionCallInfo& info) -> Value {
                if (!info.thisValue().isObject()) return Value::undefined();
                DateObject* d = dynamic_cast<DateObject*>(info.thisValue().asObject());
                if (!d) return Value::undefined();
                return Value::number(static_cast<double>(getter(d)));
            }, 0));
    };

    proto->set("getTime",         dateGetter("getTime",         [](DateObject* d) { return d->getTime(); }));
    proto->set("getFullYear",     dateGetter("getFullYear",     [](DateObject* d) { return d->getFullYear(); }));
    proto->set("getMonth",        dateGetter("getMonth",        [](DateObject* d) { return d->getMonth(); }));
    proto->set("getDate",         dateGetter("getDate",         [](DateObject* d) { return d->getDate(); }));
    proto->set("getDay",          dateGetter("getDay",          [](DateObject* d) { return d->getDay(); }));
    proto->set("getHours",        dateGetter("getHours",        [](DateObject* d) { return d->getHours(); }));
    proto->set("getMinutes",      dateGetter("getMinutes",      [](DateObject* d) { return d->getMinutes(); }));
    proto->set("getSeconds",      dateGetter("getSeconds",      [](DateObject* d) { return d->getSeconds(); }));
    proto->set("getMilliseconds", dateGetter("getMilliseconds", [](DateObject* d) { return d->getMilliseconds(); }));
    proto->set("getTimezoneOffset", dateGetter("getTimezoneOffset", [](DateObject* d) { return d->getTimezoneOffset(); }));

    // UTC getters
    proto->set("getUTCFullYear",     dateGetter("getUTCFullYear",     [](DateObject* d) { return d->getUTCFullYear(); }));
    proto->set("getUTCMonth",        dateGetter("getUTCMonth",        [](DateObject* d) { return d->getUTCMonth(); }));
    proto->set("getUTCDate",         dateGetter("getUTCDate",         [](DateObject* d) { return d->getUTCDate(); }));
    proto->set("getUTCDay",          dateGetter("getUTCDay",          [](DateObject* d) { return d->getUTCDay(); }));
    proto->set("getUTCHours",        dateGetter("getUTCHours",        [](DateObject* d) { return d->getUTCHours(); }));
    proto->set("getUTCMinutes",      dateGetter("getUTCMinutes",      [](DateObject* d) { return d->getUTCMinutes(); }));
    proto->set("getUTCSeconds",      dateGetter("getUTCSeconds",      [](DateObject* d) { return d->getUTCSeconds(); }));

    // String methods
    proto->set("toString", Value::object(new Runtime::Function("toString",
        [](const Runtime::FunctionCallInfo& info) -> Value {
            if (!info.thisValue().isObject()) return Value::undefined();
            DateObject* d = dynamic_cast<DateObject*>(info.thisValue().asObject());
            return d ? Value::string(new Runtime::String(d->toString())) : Value::undefined();
        }, 0)));

    proto->set("toISOString", Value::object(new Runtime::Function("toISOString",
        [](const Runtime::FunctionCallInfo& info) -> Value {
            if (!info.thisValue().isObject()) return Value::undefined();
            DateObject* d = dynamic_cast<DateObject*>(info.thisValue().asObject());
            return d ? Value::string(new Runtime::String(d->toISOString())) : Value::undefined();
        }, 0)));

    proto->set("toUTCString", Value::object(new Runtime::Function("toUTCString",
        [](const Runtime::FunctionCallInfo& info) -> Value {
            if (!info.thisValue().isObject()) return Value::undefined();
            DateObject* d = dynamic_cast<DateObject*>(info.thisValue().asObject());
            return d ? Value::string(new Runtime::String(d->toUTCString())) : Value::undefined();
        }, 0)));

    proto->set("toDateString", Value::object(new Runtime::Function("toDateString",
        [](const Runtime::FunctionCallInfo& info) -> Value {
            if (!info.thisValue().isObject()) return Value::undefined();
            DateObject* d = dynamic_cast<DateObject*>(info.thisValue().asObject());
            return d ? Value::string(new Runtime::String(d->toDateString())) : Value::undefined();
        }, 0)));

    proto->set("toTimeString", Value::object(new Runtime::Function("toTimeString",
        [](const Runtime::FunctionCallInfo& info) -> Value {
            if (!info.thisValue().isObject()) return Value::undefined();
            DateObject* d = dynamic_cast<DateObject*>(info.thisValue().asObject());
            return d ? Value::string(new Runtime::String(d->toTimeString())) : Value::undefined();
        }, 0)));

    // valueOf returns timestamp (for numeric coercion)
    proto->set("valueOf", Value::object(new Runtime::Function("valueOf",
        [](const Runtime::FunctionCallInfo& info) -> Value {
            if (!info.thisValue().isObject()) return Value::undefined();
            DateObject* d = dynamic_cast<DateObject*>(info.thisValue().asObject());
            return d ? Value::number(d->getTime()) : Value::undefined();
        }, 0)));

    return proto;
}

} // namespace Zepra::Builtins
