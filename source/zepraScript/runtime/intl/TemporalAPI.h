/**
 * @file TemporalAPI.h
 * @brief Temporal API (TC39 Proposal)
 */

#pragma once

#include <string>
#include <chrono>
#include <ctime>
#include <optional>
#include <stdexcept>

namespace Zepra::Runtime {

// =============================================================================
// Temporal.Duration
// =============================================================================

class Duration {
public:
    Duration() = default;
    Duration(int64_t years, int64_t months = 0, int64_t weeks = 0, int64_t days = 0,
             int64_t hours = 0, int64_t minutes = 0, int64_t seconds = 0,
             int64_t milliseconds = 0, int64_t microseconds = 0, int64_t nanoseconds = 0)
        : years_(years), months_(months), weeks_(weeks), days_(days)
        , hours_(hours), minutes_(minutes), seconds_(seconds)
        , milliseconds_(milliseconds), microseconds_(microseconds), nanoseconds_(nanoseconds) {}
    
    int64_t years() const { return years_; }
    int64_t months() const { return months_; }
    int64_t weeks() const { return weeks_; }
    int64_t days() const { return days_; }
    int64_t hours() const { return hours_; }
    int64_t minutes() const { return minutes_; }
    int64_t seconds() const { return seconds_; }
    int64_t milliseconds() const { return milliseconds_; }
    
    Duration negated() const {
        return Duration(-years_, -months_, -weeks_, -days_, -hours_, -minutes_, -seconds_,
                       -milliseconds_, -microseconds_, -nanoseconds_);
    }
    
    Duration abs() const {
        return Duration(std::abs(years_), std::abs(months_), std::abs(weeks_), std::abs(days_),
                       std::abs(hours_), std::abs(minutes_), std::abs(seconds_),
                       std::abs(milliseconds_), std::abs(microseconds_), std::abs(nanoseconds_));
    }
    
    int64_t totalSeconds() const {
        return seconds_ + minutes_ * 60 + hours_ * 3600 + days_ * 86400;
    }
    
    std::string toString() const {
        std::string result = "P";
        if (years_) result += std::to_string(years_) + "Y";
        if (months_) result += std::to_string(months_) + "M";
        if (weeks_) result += std::to_string(weeks_) + "W";
        if (days_) result += std::to_string(days_) + "D";
        if (hours_ || minutes_ || seconds_) {
            result += "T";
            if (hours_) result += std::to_string(hours_) + "H";
            if (minutes_) result += std::to_string(minutes_) + "M";
            if (seconds_) result += std::to_string(seconds_) + "S";
        }
        return result.length() == 1 ? "PT0S" : result;
    }

private:
    int64_t years_ = 0, months_ = 0, weeks_ = 0, days_ = 0;
    int64_t hours_ = 0, minutes_ = 0, seconds_ = 0;
    int64_t milliseconds_ = 0, microseconds_ = 0, nanoseconds_ = 0;
};

// =============================================================================
// Temporal.PlainDate
// =============================================================================

class PlainDate {
public:
    PlainDate(int year, int month, int day)
        : year_(year), month_(month), day_(day) { validate(); }
    
    int year() const { return year_; }
    int month() const { return month_; }
    int day() const { return day_; }
    
    int dayOfWeek() const {
        // Zeller's congruence (1 = Monday, 7 = Sunday)
        int y = year_, m = month_;
        if (m < 3) { m += 12; --y; }
        int k = y % 100, j = y / 100;
        int h = (day_ + (13 * (m + 1)) / 5 + k + k / 4 + j / 4 - 2 * j) % 7;
        return ((h + 5) % 7) + 1;
    }
    
    int dayOfYear() const {
        static const int daysBeforeMonth[] = {0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
        int doy = daysBeforeMonth[month_] + day_;
        if (month_ > 2 && inLeapYear()) ++doy;
        return doy;
    }
    
    int weekOfYear() const {
        return (dayOfYear() - 1) / 7 + 1;
    }
    
    int daysInMonth() const {
        static const int days[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        if (month_ == 2 && inLeapYear()) return 29;
        return days[month_];
    }
    
    int daysInYear() const { return inLeapYear() ? 366 : 365; }
    
    bool inLeapYear() const { return (year_ % 4 == 0 && year_ % 100 != 0) || year_ % 400 == 0; }
    
    PlainDate add(const Duration& d) const {
        return PlainDate(year_ + d.years(), month_ + d.months(), day_ + d.days());
    }
    
    PlainDate subtract(const Duration& d) const {
        return add(d.negated());
    }
    
    Duration until(const PlainDate& other) const {
        return Duration(other.year_ - year_, other.month_ - month_, 0, other.day_ - day_);
    }
    
    Duration since(const PlainDate& other) const {
        return other.until(*this);
    }
    
    bool equals(const PlainDate& other) const {
        return year_ == other.year_ && month_ == other.month_ && day_ == other.day_;
    }
    
    int compare(const PlainDate& other) const {
        if (year_ != other.year_) return year_ - other.year_;
        if (month_ != other.month_) return month_ - other.month_;
        return day_ - other.day_;
    }
    
    std::string toString() const {
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d", year_, month_, day_);
        return buf;
    }

private:
    void validate() {
        if (month_ < 1 || month_ > 12 || day_ < 1 || day_ > 31) {
            throw std::out_of_range("Invalid date");
        }
    }
    
    int year_, month_, day_;
};

// =============================================================================
// Temporal.PlainTime
// =============================================================================

class PlainTime {
public:
    PlainTime(int hour = 0, int minute = 0, int second = 0, int ms = 0, int us = 0, int ns = 0)
        : hour_(hour), minute_(minute), second_(second), millisecond_(ms), microsecond_(us), nanosecond_(ns) {}
    
    int hour() const { return hour_; }
    int minute() const { return minute_; }
    int second() const { return second_; }
    int millisecond() const { return millisecond_; }
    
    PlainTime add(const Duration& d) const {
        int h = hour_ + d.hours();
        int m = minute_ + d.minutes();
        int s = second_ + d.seconds();
        m += s / 60; s %= 60;
        h += m / 60; m %= 60;
        h %= 24;
        return PlainTime(h, m, s, millisecond_);
    }
    
    std::string toString() const {
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%02d:%02d:%02d", hour_, minute_, second_);
        return buf;
    }

private:
    int hour_, minute_, second_, millisecond_, microsecond_, nanosecond_;
};

// =============================================================================
// Temporal.PlainDateTime
// =============================================================================

class PlainDateTime {
public:
    PlainDateTime(int year, int month, int day, int hour = 0, int minute = 0, int second = 0)
        : date_(year, month, day), time_(hour, minute, second) {}
    
    PlainDateTime(const PlainDate& date, const PlainTime& time = {})
        : date_(date), time_(time) {}
    
    const PlainDate& date() const { return date_; }
    const PlainTime& time() const { return time_; }
    int year() const { return date_.year(); }
    int month() const { return date_.month(); }
    int day() const { return date_.day(); }
    int hour() const { return time_.hour(); }
    int minute() const { return time_.minute(); }
    int second() const { return time_.second(); }
    
    std::string toString() const {
        return date_.toString() + "T" + time_.toString();
    }

private:
    PlainDate date_;
    PlainTime time_;
};

// =============================================================================
// Temporal.Instant
// =============================================================================

class Instant {
public:
    explicit Instant(int64_t epochNanoseconds) : epochNs_(epochNanoseconds) {}
    
    int64_t epochSeconds() const { return epochNs_ / 1'000'000'000; }
    int64_t epochMilliseconds() const { return epochNs_ / 1'000'000; }
    int64_t epochMicroseconds() const { return epochNs_ / 1'000; }
    int64_t epochNanoseconds() const { return epochNs_; }
    
    Instant add(const Duration& d) const {
        return Instant(epochNs_ + d.totalSeconds() * 1'000'000'000);
    }
    
    Duration until(const Instant& other) const {
        int64_t diffNs = other.epochNs_ - epochNs_;
        return Duration(0, 0, 0, 0, 0, 0, diffNs / 1'000'000'000);
    }
    
    std::string toString() const {
        time_t t = epochSeconds();
        char buf[32];
        std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&t));
        return buf;
    }

private:
    int64_t epochNs_;
};

// =============================================================================
// Temporal.Now
// =============================================================================

class Now {
public:
    static Instant instant() {
        auto now = std::chrono::system_clock::now();
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
        return Instant(ns);
    }
    
    static PlainDateTime plainDateTimeISO() {
        time_t t = std::time(nullptr);
        std::tm* tm = std::localtime(&t);
        return PlainDateTime(tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
                            tm->tm_hour, tm->tm_min, tm->tm_sec);
    }
    
    static PlainDate plainDateISO() {
        time_t t = std::time(nullptr);
        std::tm* tm = std::localtime(&t);
        return PlainDate(tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
    }
    
    static PlainTime plainTimeISO() {
        time_t t = std::time(nullptr);
        std::tm* tm = std::localtime(&t);
        return PlainTime(tm->tm_hour, tm->tm_min, tm->tm_sec);
    }
};

} // namespace Zepra::Runtime
