/**
 * @file InstantAPI.h
 * @brief Temporal.Instant Implementation
 */

#pragma once

#include <string>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <cmath>

namespace Zepra::Runtime {

// =============================================================================
// Temporal.Instant
// =============================================================================

class Instant {
public:
    using Duration = std::chrono::nanoseconds;
    
    explicit Instant(int64_t epochNanoseconds = 0)
        : epochNs_(epochNanoseconds) {}
    
    static Instant now() {
        auto now = std::chrono::system_clock::now();
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            now.time_since_epoch()).count();
        return Instant(ns);
    }
    
    static Instant fromEpochSeconds(int64_t seconds) {
        return Instant(seconds * 1'000'000'000LL);
    }
    
    static Instant fromEpochMilliseconds(int64_t milliseconds) {
        return Instant(milliseconds * 1'000'000LL);
    }
    
    static Instant fromEpochMicroseconds(int64_t microseconds) {
        return Instant(microseconds * 1'000LL);
    }
    
    static Instant fromEpochNanoseconds(int64_t nanoseconds) {
        return Instant(nanoseconds);
    }
    
    int64_t epochSeconds() const { return epochNs_ / 1'000'000'000LL; }
    int64_t epochMilliseconds() const { return epochNs_ / 1'000'000LL; }
    int64_t epochMicroseconds() const { return epochNs_ / 1'000LL; }
    int64_t epochNanoseconds() const { return epochNs_; }
    
    Instant add(int64_t hours = 0, int64_t minutes = 0, int64_t seconds = 0,
                int64_t milliseconds = 0, int64_t microseconds = 0, int64_t nanoseconds = 0) const {
        int64_t totalNs = epochNs_;
        totalNs += hours * 3'600'000'000'000LL;
        totalNs += minutes * 60'000'000'000LL;
        totalNs += seconds * 1'000'000'000LL;
        totalNs += milliseconds * 1'000'000LL;
        totalNs += microseconds * 1'000LL;
        totalNs += nanoseconds;
        return Instant(totalNs);
    }
    
    Instant subtract(int64_t hours = 0, int64_t minutes = 0, int64_t seconds = 0,
                     int64_t milliseconds = 0, int64_t microseconds = 0, int64_t nanoseconds = 0) const {
        return add(-hours, -minutes, -seconds, -milliseconds, -microseconds, -nanoseconds);
    }
    
    struct DurationSince {
        int64_t hours;
        int64_t minutes;
        int64_t seconds;
        int64_t milliseconds;
        int64_t microseconds;
        int64_t nanoseconds;
    };
    
    DurationSince since(const Instant& other) const {
        int64_t diff = epochNs_ - other.epochNs_;
        bool negative = diff < 0;
        int64_t abs = std::abs(diff);
        
        DurationSince d;
        d.hours = abs / 3'600'000'000'000LL;
        abs %= 3'600'000'000'000LL;
        d.minutes = abs / 60'000'000'000LL;
        abs %= 60'000'000'000LL;
        d.seconds = abs / 1'000'000'000LL;
        abs %= 1'000'000'000LL;
        d.milliseconds = abs / 1'000'000LL;
        abs %= 1'000'000LL;
        d.microseconds = abs / 1'000LL;
        d.nanoseconds = abs % 1'000LL;
        
        if (negative) {
            d.hours = -d.hours;
            d.minutes = -d.minutes;
            d.seconds = -d.seconds;
            d.milliseconds = -d.milliseconds;
            d.microseconds = -d.microseconds;
            d.nanoseconds = -d.nanoseconds;
        }
        
        return d;
    }
    
    DurationSince until(const Instant& other) const {
        return other.since(*this);
    }
    
    int compare(const Instant& other) const {
        if (epochNs_ < other.epochNs_) return -1;
        if (epochNs_ > other.epochNs_) return 1;
        return 0;
    }
    
    bool equals(const Instant& other) const {
        return epochNs_ == other.epochNs_;
    }
    
    bool operator==(const Instant& other) const { return equals(other); }
    bool operator!=(const Instant& other) const { return !equals(other); }
    bool operator<(const Instant& other) const { return compare(other) < 0; }
    bool operator<=(const Instant& other) const { return compare(other) <= 0; }
    bool operator>(const Instant& other) const { return compare(other) > 0; }
    bool operator>=(const Instant& other) const { return compare(other) >= 0; }
    
    std::string toString() const {
        std::time_t t = static_cast<std::time_t>(epochSeconds());
        std::tm* tm = std::gmtime(&t);
        
        std::ostringstream oss;
        oss << std::setfill('0')
            << std::setw(4) << (tm->tm_year + 1900) << "-"
            << std::setw(2) << (tm->tm_mon + 1) << "-"
            << std::setw(2) << tm->tm_mday << "T"
            << std::setw(2) << tm->tm_hour << ":"
            << std::setw(2) << tm->tm_min << ":"
            << std::setw(2) << tm->tm_sec;
        
        int ns = epochNs_ % 1'000'000'000LL;
        if (ns != 0) {
            oss << "." << std::setw(9) << ns;
        }
        
        oss << "Z";
        return oss.str();
    }
    
    std::string toLocaleString(const std::string& locale = "en") const {
        return toString();
    }

private:
    int64_t epochNs_;
};

} // namespace Zepra::Runtime
