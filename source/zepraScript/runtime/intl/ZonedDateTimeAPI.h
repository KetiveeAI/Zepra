/**
 * @file ZonedDateTimeAPI.h
 * @brief Temporal.ZonedDateTime Implementation
 */

#pragma once

#include <string>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>

namespace Zepra::Runtime {

// =============================================================================
// TimeZone
// =============================================================================

class TimeZoneInfo {
public:
    explicit TimeZoneInfo(const std::string& id = "UTC") : id_(id) {
        offset_ = parseOffset(id);
    }
    
    const std::string& id() const { return id_; }
    int offsetSeconds() const { return offset_; }
    int offsetMinutes() const { return offset_ / 60; }
    int offsetHours() const { return offset_ / 3600; }
    
    std::string formatOffset() const {
        int hours = std::abs(offset_) / 3600;
        int mins = (std::abs(offset_) % 3600) / 60;
        char sign = offset_ >= 0 ? '+' : '-';
        std::ostringstream oss;
        oss << sign << std::setfill('0') << std::setw(2) << hours
            << ":" << std::setw(2) << mins;
        return oss.str();
    }

private:
    int parseOffset(const std::string& tz) const {
        if (tz == "UTC" || tz == "Z") return 0;
        if (tz == "America/New_York" || tz == "EST") return -5 * 3600;
        if (tz == "America/Los_Angeles" || tz == "PST") return -8 * 3600;
        if (tz == "Europe/London" || tz == "GMT") return 0;
        if (tz == "Europe/Paris" || tz == "CET") return 1 * 3600;
        if (tz == "Asia/Tokyo" || tz == "JST") return 9 * 3600;
        if (tz == "Asia/Kolkata" || tz == "IST") return 5 * 3600 + 30 * 60;
        if (tz == "Asia/Shanghai" || tz == "CST") return 8 * 3600;
        
        // Parse +HH:MM or -HH:MM
        if (tz.length() >= 6 && (tz[0] == '+' || tz[0] == '-')) {
            int sign = tz[0] == '+' ? 1 : -1;
            int hours = std::stoi(tz.substr(1, 2));
            int mins = tz.length() >= 5 ? std::stoi(tz.substr(4, 2)) : 0;
            return sign * (hours * 3600 + mins * 60);
        }
        
        return 0;
    }
    
    std::string id_;
    int offset_;
};

// =============================================================================
// Temporal.ZonedDateTime
// =============================================================================

class ZonedDateTime {
public:
    using TimePoint = std::chrono::system_clock::time_point;
    
    ZonedDateTime(int year, int month, int day,
                  int hour = 0, int minute = 0, int second = 0,
                  int millisecond = 0, const std::string& timeZone = "UTC")
        : year_(year), month_(month), day_(day),
          hour_(hour), minute_(minute), second_(second),
          millisecond_(millisecond), timeZone_(timeZone) {}
    
    static ZonedDateTime now(const std::string& timeZone = "UTC") {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::tm* tm = std::gmtime(&time);
        
        TimeZoneInfo tz(timeZone);
        int totalSeconds = tz.offsetSeconds();
        
        int adjHour = tm->tm_hour + totalSeconds / 3600;
        int adjMin = tm->tm_min + (totalSeconds % 3600) / 60;
        
        while (adjMin >= 60) { adjHour++; adjMin -= 60; }
        while (adjMin < 0) { adjHour--; adjMin += 60; }
        while (adjHour >= 24) { adjHour -= 24; }
        while (adjHour < 0) { adjHour += 24; }
        
        return ZonedDateTime(
            tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
            adjHour, adjMin, tm->tm_sec, 0, timeZone
        );
    }
    
    static ZonedDateTime fromEpochSeconds(int64_t seconds, const std::string& timeZone = "UTC") {
        std::time_t t = static_cast<std::time_t>(seconds);
        std::tm* tm = std::gmtime(&t);
        
        TimeZoneInfo tz(timeZone);
        int adjHour = tm->tm_hour + tz.offsetHours();
        
        return ZonedDateTime(
            tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
            adjHour, tm->tm_min, tm->tm_sec, 0, timeZone
        );
    }
    
    int year() const { return year_; }
    int month() const { return month_; }
    int day() const { return day_; }
    int hour() const { return hour_; }
    int minute() const { return minute_; }
    int second() const { return second_; }
    int millisecond() const { return millisecond_; }
    const std::string& timeZone() const { return timeZone_.id(); }
    
    int64_t epochSeconds() const {
        std::tm tm = {};
        tm.tm_year = year_ - 1900;
        tm.tm_mon = month_ - 1;
        tm.tm_mday = day_;
        tm.tm_hour = hour_;
        tm.tm_min = minute_;
        tm.tm_sec = second_;
        return std::mktime(&tm) - timeZone_.offsetSeconds();
    }
    
    int64_t epochMilliseconds() const {
        return epochSeconds() * 1000 + millisecond_;
    }
    
    ZonedDateTime withTimeZone(const std::string& newTimeZone) const {
        TimeZoneInfo newTz(newTimeZone);
        int diff = newTz.offsetSeconds() - timeZone_.offsetSeconds();
        
        int newHour = hour_ + diff / 3600;
        int newMin = minute_ + (diff % 3600) / 60;
        
        while (newMin >= 60) { newHour++; newMin -= 60; }
        while (newMin < 0) { newHour--; newMin += 60; }
        
        int newDay = day_;
        if (newHour >= 24) { newHour -= 24; newDay++; }
        if (newHour < 0) { newHour += 24; newDay--; }
        
        return ZonedDateTime(year_, month_, newDay, newHour, newMin, second_, millisecond_, newTimeZone);
    }
    
    std::string toString() const {
        std::ostringstream oss;
        oss << std::setfill('0')
            << std::setw(4) << year_ << "-"
            << std::setw(2) << month_ << "-"
            << std::setw(2) << day_ << "T"
            << std::setw(2) << hour_ << ":"
            << std::setw(2) << minute_ << ":"
            << std::setw(2) << second_;
        
        if (millisecond_ > 0) {
            oss << "." << std::setw(3) << millisecond_;
        }
        
        oss << timeZone_.formatOffset() << "[" << timeZone_.id() << "]";
        return oss.str();
    }

private:
    int year_, month_, day_;
    int hour_, minute_, second_, millisecond_;
    TimeZoneInfo timeZone_;
};

} // namespace Zepra::Runtime
