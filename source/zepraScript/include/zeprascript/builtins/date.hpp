#pragma once

/**
 * @file date.hpp
 * @brief JavaScript Date object
 */

#include "../config.hpp"
#include "../runtime/object.hpp"
#include "../runtime/value.hpp"
#include <ctime>
#include <chrono>

namespace Zepra::Runtime { class Context; }

namespace Zepra::Builtins {

using Runtime::Value;
using Runtime::Object;

/**
 * @brief JavaScript Date object
 */
class DateObject : public Object {
public:
    DateObject();
    explicit DateObject(double timestamp);
    DateObject(int year, int month, int day = 1, int hours = 0, 
               int minutes = 0, int seconds = 0, int ms = 0);
    
    // Getters
    int getFullYear() const;
    int getMonth() const;
    int getDate() const;
    int getDay() const;
    int getHours() const;
    int getMinutes() const;
    int getSeconds() const;
    int getMilliseconds() const;
    double getTime() const { return timestamp_; }
    
    // UTC getters
    int getUTCFullYear() const;
    int getUTCMonth() const;
    int getUTCDate() const;
    int getUTCDay() const;
    int getUTCHours() const;
    int getUTCMinutes() const;
    int getUTCSeconds() const;
    int getUTCMilliseconds() const;
    
    // Setters
    void setFullYear(int year);
    void setMonth(int month);
    void setDate(int day);
    void setHours(int hours);
    void setMinutes(int minutes);
    void setSeconds(int seconds);
    void setMilliseconds(int ms);
    void setTime(double timestamp) { timestamp_ = timestamp; }
    
    // String representations
    std::string toString() const;
    std::string toDateString() const;
    std::string toTimeString() const;
    std::string toISOString() const;
    std::string toUTCString() const;
    std::string toLocaleString() const;
    
    // Timezone
    int getTimezoneOffset() const;
    
private:
    double timestamp_; // ms since epoch
    
    std::tm toTm() const;
    void fromTm(const std::tm& tm);
};

/**
 * @brief Date builtin methods
 */
class DateBuiltin {
public:
    static Value constructor(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value now(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value parse(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value UTC(Runtime::Context* ctx, const std::vector<Value>& args);
    
    static Value getTime(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value getFullYear(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value getMonth(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value getDate(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value getDay(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value getHours(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value getMinutes(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value getSeconds(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value toString(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value toISOString(Runtime::Context* ctx, const std::vector<Value>& args);
    
    static Object* createDatePrototype();
};

} // namespace Zepra::Builtins
