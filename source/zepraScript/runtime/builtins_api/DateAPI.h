/**
 * @file DateAPI.h
 * @brief Date Implementation
 */

#pragma once

#include <ctime>
#include <chrono>
#include <string>
#include <sstream>
#include <iomanip>
#include <optional>

namespace Zepra::Runtime {

class Date {
public:
    using TimePoint = std::chrono::system_clock::time_point;
    using Duration = std::chrono::milliseconds;
    
    Date() : timestamp_(std::chrono::system_clock::now()) {}
    explicit Date(int64_t milliseconds) 
        : timestamp_(TimePoint(Duration(milliseconds))) {}
    Date(int year, int month, int day = 1, int hours = 0, int mins = 0, int secs = 0, int ms = 0) {
        std::tm tm = {};
        tm.tm_year = year - 1900;
        tm.tm_mon = month;
        tm.tm_mday = day;
        tm.tm_hour = hours;
        tm.tm_min = mins;
        tm.tm_sec = secs;
        auto time = std::mktime(&tm);
        timestamp_ = std::chrono::system_clock::from_time_t(time) + Duration(ms);
    }
    
    // Static methods
    static int64_t now() {
        return std::chrono::duration_cast<Duration>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
    
    static int64_t UTC(int year, int month, int day = 1, int hours = 0, int mins = 0, int secs = 0, int ms = 0) {
        std::tm tm = {};
        tm.tm_year = year - 1900;
        tm.tm_mon = month;
        tm.tm_mday = day;
        tm.tm_hour = hours;
        tm.tm_min = mins;
        tm.tm_sec = secs;
        auto time = timegm(&tm);
        return time * 1000 + ms;
    }
    
    static std::optional<int64_t> parse(const std::string& dateString) {
        std::tm tm = {};
        std::istringstream ss(dateString);
        ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
        if (ss.fail()) {
            ss.clear();
            ss.str(dateString);
            ss >> std::get_time(&tm, "%Y-%m-%d");
        }
        if (ss.fail()) return std::nullopt;
        return std::mktime(&tm) * 1000;
    }
    
    // Getters (local time)
    int getFullYear() const { return localTm().tm_year + 1900; }
    int getMonth() const { return localTm().tm_mon; }
    int getDate() const { return localTm().tm_mday; }
    int getDay() const { return localTm().tm_wday; }
    int getHours() const { return localTm().tm_hour; }
    int getMinutes() const { return localTm().tm_min; }
    int getSeconds() const { return localTm().tm_sec; }
    int getMilliseconds() const { return getTime() % 1000; }
    
    // Getters (UTC)
    int getUTCFullYear() const { return utcTm().tm_year + 1900; }
    int getUTCMonth() const { return utcTm().tm_mon; }
    int getUTCDate() const { return utcTm().tm_mday; }
    int getUTCDay() const { return utcTm().tm_wday; }
    int getUTCHours() const { return utcTm().tm_hour; }
    int getUTCMinutes() const { return utcTm().tm_min; }
    int getUTCSeconds() const { return utcTm().tm_sec; }
    int getUTCMilliseconds() const { return getTime() % 1000; }
    
    int64_t getTime() const {
        return std::chrono::duration_cast<Duration>(timestamp_.time_since_epoch()).count();
    }
    
    int getTimezoneOffset() const {
        auto t = std::chrono::system_clock::to_time_t(timestamp_);
        std::tm local = *std::localtime(&t);
        std::tm utc = *std::gmtime(&t);
        return (utc.tm_hour - local.tm_hour) * 60 + (utc.tm_min - local.tm_min);
    }
    
    // Setters
    void setTime(int64_t ms) { timestamp_ = TimePoint(Duration(ms)); }
    
    void setFullYear(int year) {
        auto tm = localTm();
        tm.tm_year = year - 1900;
        timestamp_ = std::chrono::system_clock::from_time_t(std::mktime(&tm));
    }
    
    void setMonth(int month) {
        auto tm = localTm();
        tm.tm_mon = month;
        timestamp_ = std::chrono::system_clock::from_time_t(std::mktime(&tm));
    }
    
    void setDate(int day) {
        auto tm = localTm();
        tm.tm_mday = day;
        timestamp_ = std::chrono::system_clock::from_time_t(std::mktime(&tm));
    }
    
    void setHours(int hours) {
        auto tm = localTm();
        tm.tm_hour = hours;
        timestamp_ = std::chrono::system_clock::from_time_t(std::mktime(&tm));
    }
    
    void setMinutes(int minutes) {
        auto tm = localTm();
        tm.tm_min = minutes;
        timestamp_ = std::chrono::system_clock::from_time_t(std::mktime(&tm));
    }
    
    void setSeconds(int seconds) {
        auto tm = localTm();
        tm.tm_sec = seconds;
        timestamp_ = std::chrono::system_clock::from_time_t(std::mktime(&tm));
    }
    
    // String representations
    std::string toString() const {
        auto tm = localTm();
        std::ostringstream oss;
        oss << std::put_time(&tm, "%a %b %d %Y %H:%M:%S GMT%z");
        return oss.str();
    }
    
    std::string toISOString() const {
        auto tm = utcTm();
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
        oss << "." << std::setw(3) << std::setfill('0') << getMilliseconds() << "Z";
        return oss.str();
    }
    
    std::string toDateString() const {
        auto tm = localTm();
        std::ostringstream oss;
        oss << std::put_time(&tm, "%a %b %d %Y");
        return oss.str();
    }
    
    std::string toTimeString() const {
        auto tm = localTm();
        std::ostringstream oss;
        oss << std::put_time(&tm, "%H:%M:%S GMT%z");
        return oss.str();
    }
    
    std::string toJSON() const { return toISOString(); }
    int64_t valueOf() const { return getTime(); }

private:
    std::tm localTm() const {
        auto t = std::chrono::system_clock::to_time_t(timestamp_);
        return *std::localtime(&t);
    }
    
    std::tm utcTm() const {
        auto t = std::chrono::system_clock::to_time_t(timestamp_);
        return *std::gmtime(&t);
    }
    
    TimePoint timestamp_;
};

} // namespace Zepra::Runtime
