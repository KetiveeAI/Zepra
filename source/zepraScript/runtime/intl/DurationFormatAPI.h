/**
 * @file DurationFormatAPI.h
 * @brief Intl.DurationFormat Implementation
 */

#pragma once

#include <string>
#include <vector>
#include <optional>
#include <cstdint>

namespace Zepra::Runtime {

// =============================================================================
// Duration
// =============================================================================

struct DurationValue {
    int64_t years = 0;
    int64_t months = 0;
    int64_t weeks = 0;
    int64_t days = 0;
    int64_t hours = 0;
    int64_t minutes = 0;
    int64_t seconds = 0;
    int64_t milliseconds = 0;
    int64_t microseconds = 0;
    int64_t nanoseconds = 0;
};

// =============================================================================
// DurationFormat Options
// =============================================================================

enum class DurationStyle { Long, Short, Narrow, Digital };

struct DurationFormatOptions {
    std::string locale = "en";
    DurationStyle style = DurationStyle::Short;
    
    std::optional<std::string> years;
    std::optional<std::string> months;
    std::optional<std::string> weeks;
    std::optional<std::string> days;
    std::optional<std::string> hours;
    std::optional<std::string> minutes;
    std::optional<std::string> seconds;
    std::optional<std::string> milliseconds;
    std::optional<std::string> fractionalDigits;
};

// =============================================================================
// DurationFormat
// =============================================================================

class DurationFormat {
public:
    DurationFormat(const std::string& locale = "en", DurationFormatOptions options = {})
        : locale_(locale), options_(std::move(options)) {}
    
    std::string format(const DurationValue& duration) const {
        std::vector<std::string> parts;
        
        if (duration.years != 0) {
            parts.push_back(formatPart(duration.years, "year", "years"));
        }
        if (duration.months != 0) {
            parts.push_back(formatPart(duration.months, "month", "months"));
        }
        if (duration.weeks != 0) {
            parts.push_back(formatPart(duration.weeks, "week", "weeks"));
        }
        if (duration.days != 0) {
            parts.push_back(formatPart(duration.days, "day", "days"));
        }
        if (duration.hours != 0) {
            parts.push_back(formatPart(duration.hours, "hour", "hours"));
        }
        if (duration.minutes != 0) {
            parts.push_back(formatPart(duration.minutes, "minute", "minutes"));
        }
        if (duration.seconds != 0 || duration.milliseconds != 0) {
            if (duration.milliseconds != 0) {
                double secs = duration.seconds + duration.milliseconds / 1000.0;
                parts.push_back(formatDecimal(secs, "second", "seconds"));
            } else {
                parts.push_back(formatPart(duration.seconds, "second", "seconds"));
            }
        }
        
        if (parts.empty()) return "0 seconds";
        
        return join(parts);
    }
    
    struct FormatPart {
        std::string type;
        std::string value;
        std::string unit;
    };
    
    std::vector<FormatPart> formatToParts(const DurationValue& duration) const {
        std::vector<FormatPart> parts;
        
        if (duration.years != 0) {
            parts.push_back({"integer", std::to_string(duration.years), "year"});
            parts.push_back({"literal", " ", ""});
            parts.push_back({"unit", duration.years == 1 ? "year" : "years", "year"});
        }
        
        return parts;
    }
    
    static std::vector<std::string> supportedLocales() {
        return {"en", "en-US", "en-GB", "es", "fr", "de", "ja", "zh"};
    }

private:
    std::string formatPart(int64_t value, const std::string& singular, const std::string& plural) const {
        std::string unit = (std::abs(value) == 1) ? singular : plural;
        
        switch (options_.style) {
            case DurationStyle::Long:
                return std::to_string(value) + " " + unit;
            case DurationStyle::Short:
                return std::to_string(value) + " " + unit.substr(0, 3);
            case DurationStyle::Narrow:
                return std::to_string(value) + unit[0];
            case DurationStyle::Digital:
                return std::to_string(value);
            default:
                return std::to_string(value) + " " + unit;
        }
    }
    
    std::string formatDecimal(double value, const std::string& singular, const std::string& plural) const {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%.3f", value);
        std::string unit = (value == 1.0) ? singular : plural;
        return std::string(buf) + " " + unit;
    }
    
    std::string join(const std::vector<std::string>& parts) const {
        std::string result;
        for (size_t i = 0; i < parts.size(); ++i) {
            if (i > 0) {
                if (i == parts.size() - 1) {
                    result += " and ";
                } else {
                    result += ", ";
                }
            }
            result += parts[i];
        }
        return result;
    }
    
    std::string locale_;
    DurationFormatOptions options_;
};

} // namespace Zepra::Runtime
