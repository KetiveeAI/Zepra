/**
 * @file temporal.cpp
 * @brief Temporal API implementation (TC39 Stage 3)
 */

#include "builtins/temporal.hpp"
#include "runtime/objects/function.hpp"
#include <sstream>
#include <iomanip>
#include <ctime>
#include <cmath>

namespace Zepra::Builtins {

// Helper: check if year is leap year
static bool isLeapYear(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

// Helper: days in month
static int daysInMonth(int year, int month) {
    static const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2 && isLeapYear(year)) return 29;
    return days[month - 1];
}

// Helper: day of year
static int dayOfYear(int year, int month, int day) {
    int doy = day;
    for (int m = 1; m < month; m++) {
        doy += daysInMonth(year, m);
    }
    return doy;
}

// Helper: day of week (1=Monday, 7=Sunday, ISO 8601)
static int dayOfWeek(int year, int month, int day) {
    // Zeller's congruence adapted for ISO week
    if (month < 3) {
        month += 12;
        year--;
    }
    int k = year % 100;
    int j = year / 100;
    int h = (day + (13 * (month + 1)) / 5 + k + k / 4 + j / 4 - 2 * j) % 7;
    // Convert to ISO (1=Mon, 7=Sun)
    return ((h + 5) % 7) + 1;
}

// =============================================================================
// TemporalInstant
// =============================================================================

TemporalInstant::TemporalInstant(int64_t epochNanoseconds)
    : Runtime::Object(Runtime::ObjectType::Date)
    , epochNanos_(epochNanoseconds) {}

TemporalInstant* TemporalInstant::now() {
    auto now = std::chrono::system_clock::now();
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        now.time_since_epoch()).count();
    return new TemporalInstant(ns);
}

TemporalInstant* TemporalInstant::fromEpochSeconds(int64_t seconds) {
    return new TemporalInstant(seconds * 1000000000LL);
}

TemporalInstant* TemporalInstant::fromEpochMilliseconds(int64_t ms) {
    return new TemporalInstant(ms * 1000000LL);
}

TemporalInstant* TemporalInstant::fromEpochMicroseconds(int64_t us) {
    return new TemporalInstant(us * 1000LL);
}

TemporalInstant* TemporalInstant::fromEpochNanoseconds(int64_t ns) {
    return new TemporalInstant(ns);
}

TemporalInstant* TemporalInstant::from(const std::string&) {
    // Simplified: returns current time
    return now();
}

int64_t TemporalInstant::epochSeconds() const {
    return epochNanos_ / 1000000000LL;
}

int64_t TemporalInstant::epochMilliseconds() const {
    return epochNanos_ / 1000000LL;
}

int64_t TemporalInstant::epochMicroseconds() const {
    return epochNanos_ / 1000LL;
}

int TemporalInstant::compare(const TemporalInstant& other) const {
    if (epochNanos_ < other.epochNanos_) return -1;
    if (epochNanos_ > other.epochNanos_) return 1;
    return 0;
}

bool TemporalInstant::equals(const TemporalInstant& other) const {
    return epochNanos_ == other.epochNanos_;
}

std::string TemporalInstant::toString() const {
    time_t secs = static_cast<time_t>(epochSeconds());
    std::tm* tm = std::gmtime(&secs);
    std::ostringstream oss;
    oss << std::setfill('0')
        << std::setw(4) << (tm->tm_year + 1900) << "-"
        << std::setw(2) << (tm->tm_mon + 1) << "-"
        << std::setw(2) << tm->tm_mday << "T"
        << std::setw(2) << tm->tm_hour << ":"
        << std::setw(2) << tm->tm_min << ":"
        << std::setw(2) << tm->tm_sec << "Z";
    return oss.str();
}

// =============================================================================
// TemporalPlainDate
// =============================================================================

TemporalPlainDate::TemporalPlainDate(int year, int month, int day)
    : Runtime::Object(Runtime::ObjectType::Date)
    , year_(year), month_(month), day_(day) {}

TemporalPlainDate* TemporalPlainDate::from(const std::string& isoString) {
    int y = 0, m = 1, d = 1;
    std::sscanf(isoString.c_str(), "%d-%d-%d", &y, &m, &d);
    return new TemporalPlainDate(y, m, d);
}

int TemporalPlainDate::dayOfWeek() const {
    return Zepra::Builtins::dayOfWeek(year_, month_, day_);
}

int TemporalPlainDate::dayOfYear() const {
    return Zepra::Builtins::dayOfYear(year_, month_, day_);
}

int TemporalPlainDate::weekOfYear() const {
    int doy = dayOfYear();
    int dow = dayOfWeek();
    return (doy - dow + 10) / 7;
}

int TemporalPlainDate::daysInMonth() const {
    return Zepra::Builtins::daysInMonth(year_, month_);
}

int TemporalPlainDate::daysInYear() const {
    return isLeapYear(year_) ? 366 : 365;
}

bool TemporalPlainDate::inLeapYear() const {
    return isLeapYear(year_);
}

int TemporalPlainDate::compare(const TemporalPlainDate& other) const {
    if (year_ != other.year_) return year_ < other.year_ ? -1 : 1;
    if (month_ != other.month_) return month_ < other.month_ ? -1 : 1;
    if (day_ != other.day_) return day_ < other.day_ ? -1 : 1;
    return 0;
}

bool TemporalPlainDate::equals(const TemporalPlainDate& other) const {
    return compare(other) == 0;
}

TemporalPlainDate* TemporalPlainDate::withYear(int year) const {
    return new TemporalPlainDate(year, month_, day_);
}

TemporalPlainDate* TemporalPlainDate::withMonth(int month) const {
    return new TemporalPlainDate(year_, month, day_);
}

TemporalPlainDate* TemporalPlainDate::withDay(int day) const {
    return new TemporalPlainDate(year_, month_, day);
}

std::string TemporalPlainDate::toString() const {
    std::ostringstream oss;
    oss << std::setfill('0')
        << std::setw(4) << year_ << "-"
        << std::setw(2) << month_ << "-"
        << std::setw(2) << day_;
    return oss.str();
}

// =============================================================================
// TemporalPlainTime
// =============================================================================

TemporalPlainTime::TemporalPlainTime(int hour, int minute, int second,
                                     int millisecond, int microsecond, int nanosecond)
    : Runtime::Object(Runtime::ObjectType::Date)
    , hour_(hour), minute_(minute), second_(second)
    , millisecond_(millisecond), microsecond_(microsecond), nanosecond_(nanosecond) {}

TemporalPlainTime* TemporalPlainTime::from(const std::string& isoString) {
    int h = 0, m = 0, s = 0;
    std::sscanf(isoString.c_str(), "%d:%d:%d", &h, &m, &s);
    return new TemporalPlainTime(h, m, s);
}

int TemporalPlainTime::compare(const TemporalPlainTime& other) const {
    if (hour_ != other.hour_) return hour_ < other.hour_ ? -1 : 1;
    if (minute_ != other.minute_) return minute_ < other.minute_ ? -1 : 1;
    if (second_ != other.second_) return second_ < other.second_ ? -1 : 1;
    if (millisecond_ != other.millisecond_) 
        return millisecond_ < other.millisecond_ ? -1 : 1;
    if (microsecond_ != other.microsecond_) 
        return microsecond_ < other.microsecond_ ? -1 : 1;
    if (nanosecond_ != other.nanosecond_) 
        return nanosecond_ < other.nanosecond_ ? -1 : 1;
    return 0;
}

bool TemporalPlainTime::equals(const TemporalPlainTime& other) const {
    return compare(other) == 0;
}

TemporalPlainTime* TemporalPlainTime::withHour(int hour) const {
    return new TemporalPlainTime(hour, minute_, second_, 
                                  millisecond_, microsecond_, nanosecond_);
}

TemporalPlainTime* TemporalPlainTime::withMinute(int minute) const {
    return new TemporalPlainTime(hour_, minute, second_,
                                  millisecond_, microsecond_, nanosecond_);
}

TemporalPlainTime* TemporalPlainTime::withSecond(int second) const {
    return new TemporalPlainTime(hour_, minute_, second,
                                  millisecond_, microsecond_, nanosecond_);
}

std::string TemporalPlainTime::toString() const {
    std::ostringstream oss;
    oss << std::setfill('0')
        << std::setw(2) << hour_ << ":"
        << std::setw(2) << minute_ << ":"
        << std::setw(2) << second_;
    if (millisecond_ || microsecond_ || nanosecond_) {
        int totalNanos = millisecond_ * 1000000 + microsecond_ * 1000 + nanosecond_;
        oss << "." << std::setw(9) << totalNanos;
    }
    return oss.str();
}

// =============================================================================
// TemporalPlainDateTime
// =============================================================================

TemporalPlainDateTime::TemporalPlainDateTime(int year, int month, int day,
                                              int hour, int minute, int second,
                                              int millisecond, int microsecond, int nanosecond)
    : Runtime::Object(Runtime::ObjectType::Date)
    , year_(year), month_(month), day_(day)
    , hour_(hour), minute_(minute), second_(second)
    , millisecond_(millisecond), microsecond_(microsecond), nanosecond_(nanosecond) {}

TemporalPlainDateTime* TemporalPlainDateTime::from(const std::string& str) {
    int y = 0, mo = 1, d = 1, h = 0, mi = 0, s = 0;
    std::sscanf(str.c_str(), "%d-%d-%dT%d:%d:%d", &y, &mo, &d, &h, &mi, &s);
    return new TemporalPlainDateTime(y, mo, d, h, mi, s);
}

int TemporalPlainDateTime::dayOfWeek() const {
    return Zepra::Builtins::dayOfWeek(year_, month_, day_);
}

int TemporalPlainDateTime::dayOfYear() const {
    return Zepra::Builtins::dayOfYear(year_, month_, day_);
}

int TemporalPlainDateTime::compare(const TemporalPlainDateTime& other) const {
    if (year_ != other.year_) return year_ < other.year_ ? -1 : 1;
    if (month_ != other.month_) return month_ < other.month_ ? -1 : 1;
    if (day_ != other.day_) return day_ < other.day_ ? -1 : 1;
    if (hour_ != other.hour_) return hour_ < other.hour_ ? -1 : 1;
    if (minute_ != other.minute_) return minute_ < other.minute_ ? -1 : 1;
    if (second_ != other.second_) return second_ < other.second_ ? -1 : 1;
    return 0;
}

bool TemporalPlainDateTime::equals(const TemporalPlainDateTime& other) const {
    return compare(other) == 0;
}

TemporalPlainDate* TemporalPlainDateTime::toPlainDate() const {
    return new TemporalPlainDate(year_, month_, day_);
}

TemporalPlainTime* TemporalPlainDateTime::toPlainTime() const {
    return new TemporalPlainTime(hour_, minute_, second_,
                                  millisecond_, microsecond_, nanosecond_);
}

std::string TemporalPlainDateTime::toString() const {
    std::ostringstream oss;
    oss << std::setfill('0')
        << std::setw(4) << year_ << "-"
        << std::setw(2) << month_ << "-"
        << std::setw(2) << day_ << "T"
        << std::setw(2) << hour_ << ":"
        << std::setw(2) << minute_ << ":"
        << std::setw(2) << second_;
    return oss.str();
}

// =============================================================================
// TemporalDuration
// =============================================================================

TemporalDuration::TemporalDuration(int years, int months, int weeks, int days,
                                   int hours, int minutes, int seconds,
                                   int milliseconds, int microseconds, int nanoseconds)
    : Runtime::Object(Runtime::ObjectType::Ordinary)
    , years_(years), months_(months), weeks_(weeks), days_(days)
    , hours_(hours), minutes_(minutes), seconds_(seconds)
    , milliseconds_(milliseconds), microseconds_(microseconds), nanoseconds_(nanoseconds) {}

TemporalDuration* TemporalDuration::from(const std::string&) {
    // ISO 8601 duration parsing (simplified)
    return new TemporalDuration();
}

int TemporalDuration::sign() const {
    if (years_ > 0 || months_ > 0 || weeks_ > 0 || days_ > 0 ||
        hours_ > 0 || minutes_ > 0 || seconds_ > 0 ||
        milliseconds_ > 0 || microseconds_ > 0 || nanoseconds_ > 0) return 1;
    if (years_ < 0 || months_ < 0 || weeks_ < 0 || days_ < 0 ||
        hours_ < 0 || minutes_ < 0 || seconds_ < 0 ||
        milliseconds_ < 0 || microseconds_ < 0 || nanoseconds_ < 0) return -1;
    return 0;
}

bool TemporalDuration::blank() const {
    return sign() == 0;
}

TemporalDuration* TemporalDuration::add(const TemporalDuration& other) const {
    return new TemporalDuration(
        years_ + other.years_,
        months_ + other.months_,
        weeks_ + other.weeks_,
        days_ + other.days_,
        hours_ + other.hours_,
        minutes_ + other.minutes_,
        seconds_ + other.seconds_,
        milliseconds_ + other.milliseconds_,
        microseconds_ + other.microseconds_,
        nanoseconds_ + other.nanoseconds_
    );
}

TemporalDuration* TemporalDuration::subtract(const TemporalDuration& other) const {
    return new TemporalDuration(
        years_ - other.years_,
        months_ - other.months_,
        weeks_ - other.weeks_,
        days_ - other.days_,
        hours_ - other.hours_,
        minutes_ - other.minutes_,
        seconds_ - other.seconds_,
        milliseconds_ - other.milliseconds_,
        microseconds_ - other.microseconds_,
        nanoseconds_ - other.nanoseconds_
    );
}

TemporalDuration* TemporalDuration::negated() const {
    return new TemporalDuration(
        -years_, -months_, -weeks_, -days_,
        -hours_, -minutes_, -seconds_,
        -milliseconds_, -microseconds_, -nanoseconds_
    );
}

TemporalDuration* TemporalDuration::abs() const {
    return new TemporalDuration(
        std::abs(years_), std::abs(months_), std::abs(weeks_), std::abs(days_),
        std::abs(hours_), std::abs(minutes_), std::abs(seconds_),
        std::abs(milliseconds_), std::abs(microseconds_), std::abs(nanoseconds_)
    );
}

int64_t TemporalDuration::totalNanoseconds() const {
    // Ignores years/months (calendar-dependent)
    int64_t total = 0;
    total += static_cast<int64_t>(weeks_) * 7 * 24 * 60 * 60 * 1000000000LL;
    total += static_cast<int64_t>(days_) * 24 * 60 * 60 * 1000000000LL;
    total += static_cast<int64_t>(hours_) * 60 * 60 * 1000000000LL;
    total += static_cast<int64_t>(minutes_) * 60 * 1000000000LL;
    total += static_cast<int64_t>(seconds_) * 1000000000LL;
    total += static_cast<int64_t>(milliseconds_) * 1000000LL;
    total += static_cast<int64_t>(microseconds_) * 1000LL;
    total += nanoseconds_;
    return total;
}

std::string TemporalDuration::toString() const {
    std::ostringstream oss;
    oss << "P";
    if (years_) oss << years_ << "Y";
    if (months_) oss << months_ << "M";
    if (weeks_) oss << weeks_ << "W";
    if (days_) oss << days_ << "D";
    if (hours_ || minutes_ || seconds_ || milliseconds_ || microseconds_ || nanoseconds_) {
        oss << "T";
        if (hours_) oss << hours_ << "H";
        if (minutes_) oss << minutes_ << "M";
        if (seconds_ || milliseconds_ || microseconds_ || nanoseconds_) {
            oss << seconds_;
            if (milliseconds_ || microseconds_ || nanoseconds_) {
                oss << "." << std::setfill('0') << std::setw(3) << milliseconds_;
            }
            oss << "S";
        }
    }
    if (oss.str() == "P") oss << "T0S";
    return oss.str();
}

// =============================================================================
// TemporalBuiltin - Register Temporal namespace
// =============================================================================

Runtime::Object* TemporalBuiltin::createTemporalObject(Runtime::Context*) {
    Runtime::Object* temporal = new Runtime::Object();
    
    // Temporal.Now object
    Runtime::Object* now = new Runtime::Object();
    now->set("instant", Runtime::Value::object(
        new Runtime::Function("instant", [](const Runtime::FunctionCallInfo&) -> Runtime::Value {
            return Runtime::Value::object(TemporalInstant::now());
        }, 0)));
    now->set("timeZoneId", Runtime::Value::object(
        new Runtime::Function("timeZoneId", [](const Runtime::FunctionCallInfo&) -> Runtime::Value {
            return Runtime::Value::string(new Runtime::String("UTC"));
        }, 0)));
    temporal->set("Now", Runtime::Value::object(now));
    
    // Temporal.Instant constructor
    temporal->set("Instant", Runtime::Value::object(
        new Runtime::Function("Instant", [](const Runtime::FunctionCallInfo& info) -> Runtime::Value {
            if (info.argumentCount() < 1) {
                return Runtime::Value::object(TemporalInstant::now());
            }
            int64_t ns = static_cast<int64_t>(info.argument(0).toNumber());
            return Runtime::Value::object(new TemporalInstant(ns));
        }, 1)));
    
    // Temporal.PlainDate
    temporal->set("PlainDate", Runtime::Value::object(
        new Runtime::Function("PlainDate", [](const Runtime::FunctionCallInfo& info) -> Runtime::Value {
            int year = info.argumentCount() > 0 ? static_cast<int>(info.argument(0).toNumber()) : 1970;
            int month = info.argumentCount() > 1 ? static_cast<int>(info.argument(1).toNumber()) : 1;
            int day = info.argumentCount() > 2 ? static_cast<int>(info.argument(2).toNumber()) : 1;
            return Runtime::Value::object(new TemporalPlainDate(year, month, day));
        }, 3)));
    
    // Temporal.PlainTime
    temporal->set("PlainTime", Runtime::Value::object(
        new Runtime::Function("PlainTime", [](const Runtime::FunctionCallInfo& info) -> Runtime::Value {
            int hour = info.argumentCount() > 0 ? static_cast<int>(info.argument(0).toNumber()) : 0;
            int min = info.argumentCount() > 1 ? static_cast<int>(info.argument(1).toNumber()) : 0;
            int sec = info.argumentCount() > 2 ? static_cast<int>(info.argument(2).toNumber()) : 0;
            return Runtime::Value::object(new TemporalPlainTime(hour, min, sec));
        }, 3)));
    
    // Temporal.PlainDateTime
    temporal->set("PlainDateTime", Runtime::Value::object(
        new Runtime::Function("PlainDateTime", [](const Runtime::FunctionCallInfo& info) -> Runtime::Value {
            int year = info.argumentCount() > 0 ? static_cast<int>(info.argument(0).toNumber()) : 1970;
            int month = info.argumentCount() > 1 ? static_cast<int>(info.argument(1).toNumber()) : 1;
            int day = info.argumentCount() > 2 ? static_cast<int>(info.argument(2).toNumber()) : 1;
            int hour = info.argumentCount() > 3 ? static_cast<int>(info.argument(3).toNumber()) : 0;
            int min = info.argumentCount() > 4 ? static_cast<int>(info.argument(4).toNumber()) : 0;
            int sec = info.argumentCount() > 5 ? static_cast<int>(info.argument(5).toNumber()) : 0;
            return Runtime::Value::object(new TemporalPlainDateTime(year, month, day, hour, min, sec));
        }, 6)));
    
    // Temporal.Duration
    temporal->set("Duration", Runtime::Value::object(
        new Runtime::Function("Duration", [](const Runtime::FunctionCallInfo& info) -> Runtime::Value {
            int years = info.argumentCount() > 0 ? static_cast<int>(info.argument(0).toNumber()) : 0;
            int months = info.argumentCount() > 1 ? static_cast<int>(info.argument(1).toNumber()) : 0;
            int weeks = info.argumentCount() > 2 ? static_cast<int>(info.argument(2).toNumber()) : 0;
            int days = info.argumentCount() > 3 ? static_cast<int>(info.argument(3).toNumber()) : 0;
            int hours = info.argumentCount() > 4 ? static_cast<int>(info.argument(4).toNumber()) : 0;
            int mins = info.argumentCount() > 5 ? static_cast<int>(info.argument(5).toNumber()) : 0;
            int secs = info.argumentCount() > 6 ? static_cast<int>(info.argument(6).toNumber()) : 0;
            return Runtime::Value::object(new TemporalDuration(years, months, weeks, days, hours, mins, secs));
        }, 7)));
    
    return temporal;
}

void TemporalBuiltin::registerGlobal(Runtime::Object* global) {
    global->set("Temporal", Runtime::Value::object(createTemporalObject(nullptr)));
}

} // namespace Zepra::Builtins
