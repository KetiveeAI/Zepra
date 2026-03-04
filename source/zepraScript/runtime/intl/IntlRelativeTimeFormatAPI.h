/**
 * @file IntlRelativeTimeFormatAPI.h
 * @brief Intl.RelativeTimeFormat Implementation
 */

#pragma once

#include <string>
#include <vector>
#include <cmath>

namespace Zepra::Runtime {

// =============================================================================
// RelativeTimeFormat Style & Numeric
// =============================================================================

enum class RelativeTimeStyle { Long, Short, Narrow };
enum class RelativeTimeNumeric { Always, Auto };

// =============================================================================
// RelativeTimeFormat Options
// =============================================================================

struct RelativeTimeFormatOptions {
    std::string locale = "en";
    RelativeTimeStyle style = RelativeTimeStyle::Long;
    RelativeTimeNumeric numeric = RelativeTimeNumeric::Always;
};

// =============================================================================
// Intl.RelativeTimeFormat
// =============================================================================

class RelativeTimeFormat {
public:
    RelativeTimeFormat(const std::string& locale = "en", RelativeTimeFormatOptions options = {})
        : locale_(locale), options_(std::move(options)) {}
    
    std::string format(double value, const std::string& unit) const {
        int intVal = static_cast<int>(value);
        bool isFuture = value >= 0;
        int absVal = std::abs(intVal);
        
        if (options_.numeric == RelativeTimeNumeric::Auto) {
            auto special = getSpecialForm(intVal, unit);
            if (!special.empty()) return special;
        }
        
        std::string unitStr = getUnitString(absVal, unit);
        
        if (isFuture) {
            return "in " + std::to_string(absVal) + " " + unitStr;
        } else {
            return std::to_string(absVal) + " " + unitStr + " ago";
        }
    }
    
    struct FormatPart {
        std::string type;
        std::string value;
        std::string unit;
    };
    
    std::vector<FormatPart> formatToParts(double value, const std::string& unit) const {
        std::vector<FormatPart> parts;
        
        int intVal = static_cast<int>(value);
        bool isFuture = value >= 0;
        int absVal = std::abs(intVal);
        
        if (isFuture) {
            parts.push_back({"literal", "in ", ""});
            parts.push_back({"integer", std::to_string(absVal), unit});
            parts.push_back({"literal", " ", ""});
            parts.push_back({"unit", getUnitString(absVal, unit), unit});
        } else {
            parts.push_back({"integer", std::to_string(absVal), unit});
            parts.push_back({"literal", " ", ""});
            parts.push_back({"unit", getUnitString(absVal, unit), unit});
            parts.push_back({"literal", " ago", ""});
        }
        
        return parts;
    }
    
    struct ResolvedOptions {
        std::string locale;
        std::string style;
        std::string numeric;
        std::string numberingSystem;
    };
    
    ResolvedOptions resolvedOptions() const {
        ResolvedOptions opts;
        opts.locale = locale_;
        opts.numberingSystem = "latn";
        
        switch (options_.style) {
            case RelativeTimeStyle::Long: opts.style = "long"; break;
            case RelativeTimeStyle::Short: opts.style = "short"; break;
            case RelativeTimeStyle::Narrow: opts.style = "narrow"; break;
        }
        
        switch (options_.numeric) {
            case RelativeTimeNumeric::Always: opts.numeric = "always"; break;
            case RelativeTimeNumeric::Auto: opts.numeric = "auto"; break;
        }
        
        return opts;
    }
    
    static std::vector<std::string> supportedLocales() {
        return {"en", "en-US", "es", "fr", "de", "ja", "zh"};
    }

private:
    std::string getSpecialForm(int value, const std::string& unit) const {
        if (unit == "day") {
            if (value == -1) return "yesterday";
            if (value == 0) return "today";
            if (value == 1) return "tomorrow";
        }
        if (unit == "week") {
            if (value == -1) return "last week";
            if (value == 0) return "this week";
            if (value == 1) return "next week";
        }
        if (unit == "month") {
            if (value == -1) return "last month";
            if (value == 0) return "this month";
            if (value == 1) return "next month";
        }
        if (unit == "year") {
            if (value == -1) return "last year";
            if (value == 0) return "this year";
            if (value == 1) return "next year";
        }
        return "";
    }
    
    std::string getUnitString(int value, const std::string& unit) const {
        bool plural = value != 1;
        
        switch (options_.style) {
            case RelativeTimeStyle::Long:
                if (unit == "second") return plural ? "seconds" : "second";
                if (unit == "minute") return plural ? "minutes" : "minute";
                if (unit == "hour") return plural ? "hours" : "hour";
                if (unit == "day") return plural ? "days" : "day";
                if (unit == "week") return plural ? "weeks" : "week";
                if (unit == "month") return plural ? "months" : "month";
                if (unit == "quarter") return plural ? "quarters" : "quarter";
                if (unit == "year") return plural ? "years" : "year";
                break;
            case RelativeTimeStyle::Short:
                if (unit == "second") return "sec.";
                if (unit == "minute") return "min.";
                if (unit == "hour") return "hr.";
                if (unit == "day") return plural ? "days" : "day";
                if (unit == "week") return "wk.";
                if (unit == "month") return "mo.";
                if (unit == "quarter") return "qtr.";
                if (unit == "year") return "yr.";
                break;
            case RelativeTimeStyle::Narrow:
                if (unit == "second") return "s";
                if (unit == "minute") return "m";
                if (unit == "hour") return "h";
                if (unit == "day") return "d";
                if (unit == "week") return "w";
                if (unit == "month") return "mo";
                if (unit == "quarter") return "q";
                if (unit == "year") return "y";
                break;
        }
        
        return unit;
    }
    
    std::string locale_;
    RelativeTimeFormatOptions options_;
};

} // namespace Zepra::Runtime
