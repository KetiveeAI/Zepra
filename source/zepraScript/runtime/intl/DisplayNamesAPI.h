/**
 * @file DisplayNamesAPI.h
 * @brief Intl.DisplayNames Implementation
 */

#pragma once

#include <string>
#include <map>
#include <optional>

namespace Zepra::Runtime {

// =============================================================================
// DisplayNames Options
// =============================================================================

enum class DisplayNamesType { Language, Region, Script, Currency, Calendar, DateTimeField };
enum class DisplayNamesStyle { Long, Short, Narrow };
enum class DisplayNamesFallback { Code, None };

struct DisplayNamesOptions {
    std::string locale = "en";
    DisplayNamesType type = DisplayNamesType::Language;
    DisplayNamesStyle style = DisplayNamesStyle::Long;
    DisplayNamesFallback fallback = DisplayNamesFallback::Code;
};

// =============================================================================
// DisplayNames
// =============================================================================

class DisplayNames {
public:
    DisplayNames(const std::string& locale, DisplayNamesOptions options = {})
        : locale_(locale), options_(std::move(options)) {
        initData();
    }
    
    std::optional<std::string> of(const std::string& code) const {
        const std::map<std::string, std::string>* data = nullptr;
        
        switch (options_.type) {
            case DisplayNamesType::Language: data = &languages_; break;
            case DisplayNamesType::Region: data = &regions_; break;
            case DisplayNamesType::Script: data = &scripts_; break;
            case DisplayNamesType::Currency: data = &currencies_; break;
            case DisplayNamesType::Calendar: data = &calendars_; break;
            case DisplayNamesType::DateTimeField: data = &dateTimeFields_; break;
        }
        
        if (!data) return std::nullopt;
        
        auto it = data->find(code);
        if (it != data->end()) {
            return it->second;
        }
        
        if (options_.fallback == DisplayNamesFallback::Code) {
            return code;
        }
        return std::nullopt;
    }
    
    static std::vector<std::string> supportedLocales() {
        return {"en", "en-US", "es", "fr", "de", "ja", "zh"};
    }

private:
    void initData() {
        languages_ = {
            {"en", "English"}, {"es", "Spanish"}, {"fr", "French"},
            {"de", "German"}, {"it", "Italian"}, {"pt", "Portuguese"},
            {"ru", "Russian"}, {"ja", "Japanese"}, {"ko", "Korean"},
            {"zh", "Chinese"}, {"ar", "Arabic"}, {"hi", "Hindi"},
            {"bn", "Bengali"}, {"pa", "Punjabi"}, {"te", "Telugu"},
            {"mr", "Marathi"}, {"ta", "Tamil"}, {"ur", "Urdu"},
            {"gu", "Gujarati"}, {"kn", "Kannada"}, {"ml", "Malayalam"}
        };
        
        regions_ = {
            {"US", "United States"}, {"GB", "United Kingdom"}, {"CA", "Canada"},
            {"AU", "Australia"}, {"IN", "India"}, {"DE", "Germany"},
            {"FR", "France"}, {"ES", "Spain"}, {"IT", "Italy"},
            {"JP", "Japan"}, {"KR", "South Korea"}, {"CN", "China"},
            {"BR", "Brazil"}, {"MX", "Mexico"}, {"RU", "Russia"}
        };
        
        scripts_ = {
            {"Latn", "Latin"}, {"Cyrl", "Cyrillic"}, {"Arab", "Arabic"},
            {"Deva", "Devanagari"}, {"Hans", "Simplified Chinese"},
            {"Hant", "Traditional Chinese"}, {"Jpan", "Japanese"},
            {"Kore", "Korean"}, {"Grek", "Greek"}, {"Hebr", "Hebrew"}
        };
        
        currencies_ = {
            {"USD", "US Dollar"}, {"EUR", "Euro"}, {"GBP", "British Pound"},
            {"JPY", "Japanese Yen"}, {"CNY", "Chinese Yuan"}, {"INR", "Indian Rupee"},
            {"AUD", "Australian Dollar"}, {"CAD", "Canadian Dollar"},
            {"CHF", "Swiss Franc"}, {"KRW", "South Korean Won"}
        };
        
        calendars_ = {
            {"gregory", "Gregorian Calendar"}, {"buddhist", "Buddhist Calendar"},
            {"chinese", "Chinese Calendar"}, {"hebrew", "Hebrew Calendar"},
            {"islamic", "Islamic Calendar"}, {"japanese", "Japanese Calendar"},
            {"persian", "Persian Calendar"}
        };
        
        dateTimeFields_ = {
            {"era", "era"}, {"year", "year"}, {"quarter", "quarter"},
            {"month", "month"}, {"week", "week"}, {"weekday", "day of the week"},
            {"day", "day"}, {"dayPeriod", "AM/PM"}, {"hour", "hour"},
            {"minute", "minute"}, {"second", "second"}, {"timeZoneName", "time zone"}
        };
    }
    
    std::string locale_;
    DisplayNamesOptions options_;
    std::map<std::string, std::string> languages_;
    std::map<std::string, std::string> regions_;
    std::map<std::string, std::string> scripts_;
    std::map<std::string, std::string> currencies_;
    std::map<std::string, std::string> calendars_;
    std::map<std::string, std::string> dateTimeFields_;
};

} // namespace Zepra::Runtime
