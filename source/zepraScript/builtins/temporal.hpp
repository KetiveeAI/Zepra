/**
 * @file temporal.hpp
 * @brief Temporal API implementation (TC39 Stage 3)
 * 
 * Provides modern date/time handling to replace the legacy Date object.
 * Implements key Temporal types: Instant, PlainDate, PlainTime, PlainDateTime, Duration
 */

#pragma once

#include "../config.hpp"
#include "runtime/objects/object.hpp"
#include <string>
#include <chrono>
#include <cstdint>

namespace Zepra::Runtime {
    class Context;  // Forward declaration
}

namespace Zepra::Builtins {

// Forward declarations
class TemporalInstant;
class TemporalPlainDate;
class TemporalPlainTime;
class TemporalPlainDateTime;
class TemporalDuration;
class TemporalZonedDateTime;

/**
 * @brief Temporal.Instant - exact moment in time (nanoseconds since epoch)
 */
class TemporalInstant : public Runtime::Object {
public:
    explicit TemporalInstant(int64_t epochNanoseconds);
    
    // Factory methods
    static TemporalInstant* now();
    static TemporalInstant* fromEpochSeconds(int64_t seconds);
    static TemporalInstant* fromEpochMilliseconds(int64_t ms);
    static TemporalInstant* fromEpochMicroseconds(int64_t us);
    static TemporalInstant* fromEpochNanoseconds(int64_t ns);
    static TemporalInstant* from(const std::string& isoString);
    
    // Getters
    int64_t epochSeconds() const;
    int64_t epochMilliseconds() const;
    int64_t epochMicroseconds() const;
    int64_t epochNanoseconds() const { return epochNanos_; }
    
    // Comparison
    int compare(const TemporalInstant& other) const;
    bool equals(const TemporalInstant& other) const;
    
    // Arithmetic
    TemporalInstant* add(const TemporalDuration& duration) const;
    TemporalInstant* subtract(const TemporalDuration& duration) const;
    TemporalDuration* since(const TemporalInstant& other) const;
    TemporalDuration* until(const TemporalInstant& other) const;
    
    // Conversion
    std::string toString() const;
    std::string toJSON() const { return toString(); }
    
private:
    int64_t epochNanos_;
};

/**
 * @brief Temporal.PlainDate - calendar date without time
 */
class TemporalPlainDate : public Runtime::Object {
public:
    TemporalPlainDate(int year, int month, int day);
    
    // Factory
    static TemporalPlainDate* from(const std::string& isoString);
    
    // Getters
    int year() const { return year_; }
    int month() const { return month_; }
    int day() const { return day_; }
    int dayOfWeek() const;
    int dayOfYear() const;
    int weekOfYear() const;
    int daysInMonth() const;
    int daysInYear() const;
    bool inLeapYear() const;
    
    // Comparison
    int compare(const TemporalPlainDate& other) const;
    bool equals(const TemporalPlainDate& other) const;
    
    // Arithmetic
    TemporalPlainDate* add(const TemporalDuration& duration) const;
    TemporalPlainDate* subtract(const TemporalDuration& duration) const;
    TemporalDuration* since(const TemporalPlainDate& other) const;
    TemporalDuration* until(const TemporalPlainDate& other) const;
    
    // With
    TemporalPlainDate* withYear(int year) const;
    TemporalPlainDate* withMonth(int month) const;
    TemporalPlainDate* withDay(int day) const;
    
    // Conversion
    TemporalPlainDateTime* toPlainDateTime(const TemporalPlainTime* time = nullptr) const;
    std::string toString() const;
    std::string toJSON() const { return toString(); }
    
private:
    int year_;
    int month_;  // 1-12
    int day_;    // 1-31
};

/**
 * @brief Temporal.PlainTime - wall-clock time without date
 */
class TemporalPlainTime : public Runtime::Object {
public:
    TemporalPlainTime(int hour = 0, int minute = 0, int second = 0, 
                      int millisecond = 0, int microsecond = 0, int nanosecond = 0);
    
    // Factory
    static TemporalPlainTime* from(const std::string& isoString);
    
    // Getters
    int hour() const { return hour_; }
    int minute() const { return minute_; }
    int second() const { return second_; }
    int millisecond() const { return millisecond_; }
    int microsecond() const { return microsecond_; }
    int nanosecond() const { return nanosecond_; }
    
    // Comparison
    int compare(const TemporalPlainTime& other) const;
    bool equals(const TemporalPlainTime& other) const;
    
    // Arithmetic
    TemporalPlainTime* add(const TemporalDuration& duration) const;
    TemporalPlainTime* subtract(const TemporalDuration& duration) const;
    
    // With
    TemporalPlainTime* withHour(int hour) const;
    TemporalPlainTime* withMinute(int minute) const;
    TemporalPlainTime* withSecond(int second) const;
    
    // Conversion
    std::string toString() const;
    std::string toJSON() const { return toString(); }
    
private:
    int hour_;        // 0-23
    int minute_;      // 0-59
    int second_;      // 0-59
    int millisecond_; // 0-999
    int microsecond_; // 0-999
    int nanosecond_;  // 0-999
};

/**
 * @brief Temporal.PlainDateTime - date and time without timezone
 */
class TemporalPlainDateTime : public Runtime::Object {
public:
    TemporalPlainDateTime(int year, int month, int day,
                          int hour = 0, int minute = 0, int second = 0,
                          int millisecond = 0, int microsecond = 0, int nanosecond = 0);
    
    // Factory
    static TemporalPlainDateTime* from(const std::string& isoString);
    
    // Date getters
    int year() const { return year_; }
    int month() const { return month_; }
    int day() const { return day_; }
    int dayOfWeek() const;
    int dayOfYear() const;
    
    // Time getters
    int hour() const { return hour_; }
    int minute() const { return minute_; }
    int second() const { return second_; }
    int millisecond() const { return millisecond_; }
    int microsecond() const { return microsecond_; }
    int nanosecond() const { return nanosecond_; }
    
    // Comparison
    int compare(const TemporalPlainDateTime& other) const;
    bool equals(const TemporalPlainDateTime& other) const;
    
    // Arithmetic
    TemporalPlainDateTime* add(const TemporalDuration& duration) const;
    TemporalPlainDateTime* subtract(const TemporalDuration& duration) const;
    
    // Extraction
    TemporalPlainDate* toPlainDate() const;
    TemporalPlainTime* toPlainTime() const;
    
    // Conversion
    std::string toString() const;
    std::string toJSON() const { return toString(); }
    
private:
    int year_, month_, day_;
    int hour_, minute_, second_;
    int millisecond_, microsecond_, nanosecond_;
};

/**
 * @brief Temporal.Duration - length of time
 */
class TemporalDuration : public Runtime::Object {
public:
    TemporalDuration(int years = 0, int months = 0, int weeks = 0, int days = 0,
                     int hours = 0, int minutes = 0, int seconds = 0,
                     int milliseconds = 0, int microseconds = 0, int nanoseconds = 0);
    
    // Factory
    static TemporalDuration* from(const std::string& isoString);
    
    // Getters
    int years() const { return years_; }
    int months() const { return months_; }
    int weeks() const { return weeks_; }
    int days() const { return days_; }
    int hours() const { return hours_; }
    int minutes() const { return minutes_; }
    int seconds() const { return seconds_; }
    int milliseconds() const { return milliseconds_; }
    int microseconds() const { return microseconds_; }
    int nanoseconds() const { return nanoseconds_; }
    
    // Properties
    int sign() const;
    bool blank() const;
    
    // Arithmetic
    TemporalDuration* add(const TemporalDuration& other) const;
    TemporalDuration* subtract(const TemporalDuration& other) const;
    TemporalDuration* negated() const;
    TemporalDuration* abs() const;
    
    // Conversion
    int64_t totalNanoseconds() const;
    std::string toString() const;
    std::string toJSON() const { return toString(); }
    
private:
    int years_, months_, weeks_, days_;
    int hours_, minutes_, seconds_;
    int milliseconds_, microseconds_, nanoseconds_;
};

/**
 * @brief Temporal namespace object - registers all Temporal types
 */
class TemporalBuiltin {
public:
    static Runtime::Object* createTemporalObject(Runtime::Context* ctx);
    static void registerGlobal(Runtime::Object* global);
};

} // namespace Zepra::Builtins
