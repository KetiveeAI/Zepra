/**
 * @file intl.cpp
 * @brief Internationalization API implementation
 */

#include "builtins/intl.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <ctime>

namespace Zepra::Runtime {

// ============================================================================
// Locale
// ============================================================================

std::string Locale::toString() const {
    std::string result = language;
    if (!script.empty()) {
        result += "-" + script;
    }
    if (!region.empty()) {
        result += "-" + region;
    }
    // Add Unicode extension
    std::string ext;
    if (!calendar.empty() && calendar != "gregory") {
        ext += "-ca-" + calendar;
    }
    if (!collation.empty() && collation != "default") {
        ext += "-co-" + collation;
    }
    if (!numberingSystem.empty() && numberingSystem != "latn") {
        ext += "-nu-" + numberingSystem;
    }
    if (!ext.empty()) {
        result += "-u" + ext;
    }
    return result;
}

Locale Locale::parse(const std::string& tag) {
    Locale loc;
    
    // Simple BCP 47 parser
    std::vector<std::string> parts;
    std::stringstream ss(tag);
    std::string part;
    while (std::getline(ss, part, '-')) {
        parts.push_back(part);
    }
    
    if (parts.empty()) return loc;
    
    size_t idx = 0;
    
    // Language (2-3 chars)
    if (idx < parts.size() && parts[idx].length() >= 2 && parts[idx].length() <= 3) {
        loc.language = parts[idx++];
        std::transform(loc.language.begin(), loc.language.end(), 
                      loc.language.begin(), ::tolower);
    }
    
    // Script (4 chars, Title case)
    if (idx < parts.size() && parts[idx].length() == 4) {
        loc.script = parts[idx++];
        if (!loc.script.empty()) {
            loc.script[0] = std::toupper(loc.script[0]);
            for (size_t i = 1; i < loc.script.length(); i++) {
                loc.script[i] = std::tolower(loc.script[i]);
            }
        }
    }
    
    // Region (2 alpha or 3 digit)
    if (idx < parts.size() && 
        (parts[idx].length() == 2 || parts[idx].length() == 3)) {
        loc.region = parts[idx++];
        std::transform(loc.region.begin(), loc.region.end(),
                      loc.region.begin(), ::toupper);
    }
    
    // Unicode extensions (-u-)
    while (idx < parts.size()) {
        if (parts[idx] == "u" && idx + 2 < parts.size()) {
            idx++;
            while (idx + 1 < parts.size()) {
                const std::string& key = parts[idx];
                const std::string& value = parts[idx + 1];
                
                if (key == "ca") loc.calendar = value;
                else if (key == "co") loc.collation = value;
                else if (key == "nu") loc.numberingSystem = value;
                
                idx += 2;
                if (idx < parts.size() && parts[idx].length() == 2) {
                    continue;
                }
                break;
            }
        } else {
            idx++;
        }
    }
    
    // Defaults
    if (loc.calendar.empty()) loc.calendar = "gregory";
    if (loc.collation.empty()) loc.collation = "default";
    if (loc.numberingSystem.empty()) loc.numberingSystem = "latn";
    
    return loc;
}

std::vector<std::string> Locale::getCanonicalLocales(
    const std::vector<std::string>& locales) {
    std::vector<std::string> result;
    result.reserve(locales.size());
    
    for (const auto& loc : locales) {
        Locale parsed = parse(loc);
        if (!parsed.language.empty()) {
            result.push_back(parsed.toString());
        }
    }
    
    return result;
}

// ============================================================================
// IntlCollator
// ============================================================================

IntlCollator::IntlCollator(const std::vector<std::string>& locales, 
                           const CollatorOptions& options)
    : Object(ObjectType::Ordinary)
    , options_(options)
{
    // Resolve locale
    std::string resolved = locales.empty() ? 
        IntlUtils::defaultLocale() : 
        IntlUtils::bestFitMatcher(locales, IntlUtils::availableLocales());
    
    locale_ = Locale::parse(resolved);
    
    // Try to create C++ locale
    try {
        std::string localeName = locale_.language;
        if (!locale_.region.empty()) {
            localeName += "_" + locale_.region;
        }
        localeName += ".UTF-8";
        cppLocale_ = std::locale(localeName);
    } catch (...) {
        cppLocale_ = std::locale("");
    }
}

int IntlCollator::compare(const std::string& x, const std::string& y) const {
    // Use C++ locale collation
    const auto& collate = std::use_facet<std::collate<char>>(cppLocale_);
    
    int result = collate.compare(
        x.data(), x.data() + x.size(),
        y.data(), y.data() + y.size()
    );
    
    // Apply sensitivity
    if (options_.sensitivity == "base") {
        // Case and accent insensitive - simplified
        std::string lowerX = x, lowerY = y;
        std::transform(lowerX.begin(), lowerX.end(), lowerX.begin(), ::tolower);
        std::transform(lowerY.begin(), lowerY.end(), lowerY.begin(), ::tolower);
        return lowerX.compare(lowerY);
    }
    
    return result < 0 ? -1 : (result > 0 ? 1 : 0);
}

Object* IntlCollator::resolvedOptions() const {
    Object* opts = new Object();
    opts->set("locale", Value::string(new String(locale_.toString())));
    opts->set("usage", Value::string(new String(options_.usage)));
    opts->set("sensitivity", Value::string(new String(options_.sensitivity)));
    opts->set("ignorePunctuation", Value::boolean(options_.ignorePunctuation));
    opts->set("numeric", Value::boolean(options_.numeric));
    opts->set("caseFirst", Value::string(new String(options_.caseFirst)));
    opts->set("collation", Value::string(new String(options_.collation)));
    return opts;
}

std::vector<std::string> IntlCollator::supportedLocalesOf(
    const std::vector<std::string>& locales) {
    std::vector<std::string> result;
    for (const auto& loc : locales) {
        std::string resolved = IntlUtils::lookupMatcher({loc}, IntlUtils::availableLocales());
        if (!resolved.empty()) {
            result.push_back(loc);
        }
    }
    return result;
}

// ============================================================================
// IntlNumberFormat
// ============================================================================

IntlNumberFormat::IntlNumberFormat(const std::vector<std::string>& locales,
                                   const NumberFormatOptions& options)
    : Object(ObjectType::Ordinary)
    , options_(options)
{
    std::string resolved = locales.empty() ?
        IntlUtils::defaultLocale() :
        IntlUtils::bestFitMatcher(locales, IntlUtils::availableLocales());
    
    locale_ = Locale::parse(resolved);
    
    // Set defaults based on style
    if (options_.style == "currency" && options_.minimumFractionDigits < 0) {
        const auto& info = IntlUtils::getCurrencyInfo(options_.currency);
        options_.minimumFractionDigits = info.digits;
        options_.maximumFractionDigits = info.digits;
    } else if (options_.style == "percent" && options_.minimumFractionDigits < 0) {
        options_.minimumFractionDigits = 0;
        options_.maximumFractionDigits = 0;
    } else if (options_.minimumFractionDigits < 0) {
        options_.minimumFractionDigits = 0;
        options_.maximumFractionDigits = 3;
    }
}

std::string IntlNumberFormat::format(double value) const {
    switch (options_.style[0]) {
        case 'c': return formatCurrency(value);
        case 'p': return formatPercent(value);
        case 'u': return formatUnit(value);
        default:  return formatDecimal(value);
    }
}

std::string IntlNumberFormat::formatDecimal(double value) const {
    std::ostringstream oss;
    
    // Handle special values
    if (std::isnan(value)) return "NaN";
    if (std::isinf(value)) return value > 0 ? "∞" : "-∞";
    
    // Sign handling
    bool negative = value < 0;
    if (negative) value = -value;
    
    // Apply rounding
    int fracDigits = options_.maximumFractionDigits;
    double multiplier = std::pow(10.0, fracDigits);
    value = std::round(value * multiplier) / multiplier;
    
    // Format
    oss << std::fixed << std::setprecision(fracDigits) << value;
    std::string result = oss.str();
    
    // Add grouping separators
    if (options_.useGrouping) {
        size_t dotPos = result.find('.');
        if (dotPos == std::string::npos) dotPos = result.length();
        
        std::string intPart = result.substr(0, dotPos);
        std::string fracPart = dotPos < result.length() ? result.substr(dotPos) : "";
        
        std::string grouped;
        int count = 0;
        for (int i = intPart.length() - 1; i >= 0; i--) {
            if (count > 0 && count % 3 == 0) {
                grouped = "," + grouped;
            }
            grouped = intPart[i] + grouped;
            count++;
        }
        result = grouped + fracPart;
    }
    
    // Sign
    if (negative) {
        result = "-" + result;
    } else if (options_.signDisplay == "always") {
        result = "+" + result;
    }
    
    return result;
}

std::string IntlNumberFormat::formatCurrency(double value) const {
    std::string number = formatDecimal(value);
    const auto& info = IntlUtils::getCurrencyInfo(options_.currency);
    
    std::string symbol;
    if (options_.currencyDisplay == "symbol") {
        symbol = info.symbol;
    } else if (options_.currencyDisplay == "narrowSymbol") {
        symbol = info.narrowSymbol;
    } else if (options_.currencyDisplay == "code") {
        symbol = options_.currency;
    } else {
        symbol = info.name;
    }
    
    // Locale-specific placement
    if (locale_.language == "de" || locale_.language == "fr") {
        return number + " " + symbol;
    }
    return symbol + number;
}

std::string IntlNumberFormat::formatPercent(double value) const {
    double percentValue = value * 100;
    return formatDecimal(percentValue) + "%";
}

std::string IntlNumberFormat::formatUnit(double value) const {
    std::string number = formatDecimal(value);
    const auto& info = IntlUtils::getUnitInfo(options_.unit);
    
    std::string unitStr;
    if (options_.unitDisplay == "long") {
        unitStr = info.longName;
    } else if (options_.unitDisplay == "narrow") {
        unitStr = info.narrowName;
    } else {
        unitStr = info.shortName;
    }
    
    return number + " " + unitStr;
}

std::vector<std::pair<std::string, std::string>> 
IntlNumberFormat::formatToParts(double value) const {
    std::vector<std::pair<std::string, std::string>> parts;
    
    std::string formatted = format(value);
    
    // Simplified: just return whole as literal
    parts.push_back({"literal", formatted});
    
    return parts;
}

std::string IntlNumberFormat::formatRange(double start, double end) const {
    return format(start) + " – " + format(end);
}

Object* IntlNumberFormat::resolvedOptions() const {
    Object* opts = new Object();
    opts->set("locale", Value::string(new String(locale_.toString())));
    opts->set("style", Value::string(new String(options_.style)));
    opts->set("minimumIntegerDigits", Value::number(options_.minimumIntegerDigits));
    opts->set("minimumFractionDigits", Value::number(options_.minimumFractionDigits));
    opts->set("maximumFractionDigits", Value::number(options_.maximumFractionDigits));
    opts->set("useGrouping", Value::boolean(options_.useGrouping));
    opts->set("notation", Value::string(new String(options_.notation)));
    opts->set("signDisplay", Value::string(new String(options_.signDisplay)));
    if (options_.style == "currency") {
        opts->set("currency", Value::string(new String(options_.currency)));
        opts->set("currencyDisplay", Value::string(new String(options_.currencyDisplay)));
    }
    if (options_.style == "unit") {
        opts->set("unit", Value::string(new String(options_.unit)));
        opts->set("unitDisplay", Value::string(new String(options_.unitDisplay)));
    }
    return opts;
}

std::vector<std::string> IntlNumberFormat::supportedLocalesOf(
    const std::vector<std::string>& locales) {
    return IntlCollator::supportedLocalesOf(locales);
}

// ============================================================================
// IntlDateTimeFormat
// ============================================================================

IntlDateTimeFormat::IntlDateTimeFormat(const std::vector<std::string>& locales,
                                       const DateTimeFormatOptions& options)
    : Object(ObjectType::Ordinary)
    , options_(options)
{
    std::string resolved = locales.empty() ?
        IntlUtils::defaultLocale() :
        IntlUtils::bestFitMatcher(locales, IntlUtils::availableLocales());
    
    locale_ = Locale::parse(resolved);
}

std::string IntlDateTimeFormat::format(double dateValue) const {
    // Convert JS timestamp to time_t
    time_t timestamp = static_cast<time_t>(dateValue / 1000);
    struct tm* tm = localtime(&timestamp);
    
    if (!tm) return "Invalid Date";
    
    std::ostringstream oss;
    
    // Use dateStyle/timeStyle if provided
    if (!options_.dateStyle.empty() || !options_.timeStyle.empty()) {
        if (options_.dateStyle == "full") {
            char buf[100];
            strftime(buf, sizeof(buf), "%A, %B %d, %Y", tm);
            oss << buf;
        } else if (options_.dateStyle == "long") {
            char buf[100];
            strftime(buf, sizeof(buf), "%B %d, %Y", tm);
            oss << buf;
        } else if (options_.dateStyle == "medium") {
            char buf[100];
            strftime(buf, sizeof(buf), "%b %d, %Y", tm);
            oss << buf;
        } else if (options_.dateStyle == "short") {
            char buf[100];
            strftime(buf, sizeof(buf), "%m/%d/%y", tm);
            oss << buf;
        }
        
        if (!options_.dateStyle.empty() && !options_.timeStyle.empty()) {
            oss << ", ";
        }
        
        if (options_.timeStyle == "full" || options_.timeStyle == "long") {
            char buf[100];
            strftime(buf, sizeof(buf), "%I:%M:%S %p %Z", tm);
            oss << buf;
        } else if (options_.timeStyle == "medium") {
            char buf[100];
            strftime(buf, sizeof(buf), "%I:%M:%S %p", tm);
            oss << buf;
        } else if (options_.timeStyle == "short") {
            char buf[100];
            strftime(buf, sizeof(buf), "%I:%M %p", tm);
            oss << buf;
        }
    } else {
        // Default format
        char buf[100];
        strftime(buf, sizeof(buf), "%m/%d/%Y, %I:%M:%S %p", tm);
        oss << buf;
    }
    
    return oss.str();
}

std::vector<std::pair<std::string, std::string>>
IntlDateTimeFormat::formatToParts(double dateValue) const {
    std::vector<std::pair<std::string, std::string>> parts;
    
    time_t timestamp = static_cast<time_t>(dateValue / 1000);
    struct tm* tm = localtime(&timestamp);
    
    if (!tm) {
        parts.push_back({"literal", "Invalid Date"});
        return parts;
    }
    
    char buf[20];
    
    // Month
    strftime(buf, sizeof(buf), "%m", tm);
    parts.push_back({"month", buf});
    parts.push_back({"literal", "/"});
    
    // Day
    strftime(buf, sizeof(buf), "%d", tm);
    parts.push_back({"day", buf});
    parts.push_back({"literal", "/"});
    
    // Year
    strftime(buf, sizeof(buf), "%Y", tm);
    parts.push_back({"year", buf});
    
    return parts;
}

std::string IntlDateTimeFormat::formatRange(double startDate, double endDate) const {
    return format(startDate) + " – " + format(endDate);
}

Object* IntlDateTimeFormat::resolvedOptions() const {
    Object* opts = new Object();
    opts->set("locale", Value::string(new String(locale_.toString())));
    opts->set("calendar", Value::string(new String(options_.calendar)));
    opts->set("numberingSystem", Value::string(new String(options_.numberingSystem)));
    if (!options_.timeZone.empty()) {
        opts->set("timeZone", Value::string(new String(options_.timeZone)));
    }
    if (!options_.dateStyle.empty()) {
        opts->set("dateStyle", Value::string(new String(options_.dateStyle)));
    }
    if (!options_.timeStyle.empty()) {
        opts->set("timeStyle", Value::string(new String(options_.timeStyle)));
    }
    return opts;
}

std::vector<std::string> IntlDateTimeFormat::supportedLocalesOf(
    const std::vector<std::string>& locales) {
    return IntlCollator::supportedLocalesOf(locales);
}

// ============================================================================
// IntlPluralRules
// ============================================================================

IntlPluralRules::IntlPluralRules(const std::vector<std::string>& locales,
                                 const std::map<std::string, Value>& options)
    : Object(ObjectType::Ordinary)
{
    std::string resolved = locales.empty() ?
        IntlUtils::defaultLocale() :
        IntlUtils::bestFitMatcher(locales, IntlUtils::availableLocales());
    
    locale_ = Locale::parse(resolved);
    
    auto it = options.find("type");
    if (it != options.end() && it->second.isString()) {
        type_ = it->second.asString()->value();
    }
}

std::string IntlPluralRules::select(double n) const {
    // Simplified English plural rules
    if (locale_.language == "en") {
        if (type_ == "ordinal") {
            int i = static_cast<int>(n);
            int mod10 = i % 10;
            int mod100 = i % 100;
            
            if (mod10 == 1 && mod100 != 11) return "one";
            if (mod10 == 2 && mod100 != 12) return "two";
            if (mod10 == 3 && mod100 != 13) return "few";
            return "other";
        } else {
            // Cardinal
            if (n == 1) return "one";
            return "other";
        }
    }
    
    // Default: other
    return "other";
}

std::vector<std::string> IntlPluralRules::resolvedOptions() const {
    return {"one", "other"};
}

std::vector<std::string> IntlPluralRules::supportedLocalesOf(
    const std::vector<std::string>& locales) {
    return IntlCollator::supportedLocalesOf(locales);
}

// ============================================================================
// IntlListFormat
// ============================================================================

IntlListFormat::IntlListFormat(const std::vector<std::string>& locales,
                               const std::map<std::string, Value>& options)
    : Object(ObjectType::Ordinary)
{
    std::string resolved = locales.empty() ?
        IntlUtils::defaultLocale() :
        IntlUtils::bestFitMatcher(locales, IntlUtils::availableLocales());
    
    locale_ = Locale::parse(resolved);
    
    auto it = options.find("type");
    if (it != options.end() && it->second.isString()) {
        type_ = it->second.asString()->value();
    }
    
    it = options.find("style");
    if (it != options.end() && it->second.isString()) {
        style_ = it->second.asString()->value();
    }
}

std::string IntlListFormat::format(const std::vector<std::string>& list) const {
    if (list.empty()) return "";
    if (list.size() == 1) return list[0];
    
    std::string conjunction = type_ == "disjunction" ? " or " : " and ";
    if (style_ == "narrow") conjunction = type_ == "disjunction" ? "/" : ", ";
    
    std::ostringstream oss;
    for (size_t i = 0; i < list.size(); i++) {
        if (i > 0) {
            if (i == list.size() - 1) {
                if (style_ != "narrow" && list.size() > 2) {
                    oss << ",";
                }
                oss << conjunction;
            } else {
                oss << ", ";
            }
        }
        oss << list[i];
    }
    
    return oss.str();
}

std::vector<std::pair<std::string, std::string>>
IntlListFormat::formatToParts(const std::vector<std::string>& list) const {
    std::vector<std::pair<std::string, std::string>> parts;
    
    for (size_t i = 0; i < list.size(); i++) {
        parts.push_back({"element", list[i]});
        if (i < list.size() - 1) {
            if (i == list.size() - 2) {
                parts.push_back({"literal", type_ == "disjunction" ? " or " : " and "});
            } else {
                parts.push_back({"literal", ", "});
            }
        }
    }
    
    return parts;
}

std::vector<std::string> IntlListFormat::supportedLocalesOf(
    const std::vector<std::string>& locales) {
    return IntlCollator::supportedLocalesOf(locales);
}

// ============================================================================
// IntlRelativeTimeFormat
// ============================================================================

IntlRelativeTimeFormat::IntlRelativeTimeFormat(
    const std::vector<std::string>& locales,
    const std::map<std::string, Value>& options)
    : Object(ObjectType::Ordinary)
{
    std::string resolved = locales.empty() ?
        IntlUtils::defaultLocale() :
        IntlUtils::bestFitMatcher(locales, IntlUtils::availableLocales());
    
    locale_ = Locale::parse(resolved);
    
    auto it = options.find("style");
    if (it != options.end() && it->second.isString()) {
        style_ = it->second.asString()->value();
    }
    
    it = options.find("numeric");
    if (it != options.end() && it->second.isString()) {
        numeric_ = it->second.asString()->value();
    }
}

std::string IntlRelativeTimeFormat::format(double value, const std::string& unit) const {
    bool past = value < 0;
    double absValue = std::abs(value);
    int intValue = static_cast<int>(absValue);
    
    // Handle "auto" mode for special values
    if (numeric_ == "auto") {
        if (unit == "day" && intValue == 1) {
            return past ? "yesterday" : "tomorrow";
        }
        if (unit == "day" && intValue == 0) {
            return "today";
        }
    }
    
    // Numeric format
    std::string unitStr = unit;
    if (intValue != 1) {
        unitStr += "s"; // Simple pluralization
    }
    
    if (style_ == "narrow") {
        // Short form
        std::ostringstream oss;
        oss << (past ? "-" : "+") << intValue << " " << unit[0];
        return oss.str();
    }
    
    if (past) {
        return std::to_string(intValue) + " " + unitStr + " ago";
    } else {
        return "in " + std::to_string(intValue) + " " + unitStr;
    }
}

std::vector<std::pair<std::string, std::string>>
IntlRelativeTimeFormat::formatToParts(double value, const std::string& unit) const {
    std::vector<std::pair<std::string, std::string>> parts;
    
    std::string formatted = format(value, unit);
    parts.push_back({"literal", formatted});
    
    return parts;
}

std::vector<std::string> IntlRelativeTimeFormat::supportedLocalesOf(
    const std::vector<std::string>& locales) {
    return IntlCollator::supportedLocalesOf(locales);
}

// ============================================================================
// IntlObject
// ============================================================================

IntlObject::IntlObject()
    : Object(ObjectType::Ordinary) {}

std::vector<std::string> IntlObject::getCanonicalLocales(
    const std::vector<std::string>& locales) {
    return Locale::getCanonicalLocales(locales);
}

std::vector<std::string> IntlObject::supportedValuesOf(const std::string& key) {
    if (key == "calendar") {
        return {"buddhist", "chinese", "coptic", "ethiopic", "gregory", 
                "hebrew", "indian", "islamic", "japanese", "persian"};
    }
    if (key == "collation") {
        return {"big5han", "compat", "dict", "emoji", "eor", "phonebk", 
                "pinyin", "search", "standard", "stroke", "trad"};
    }
    if (key == "currency") {
        return {"USD", "EUR", "GBP", "JPY", "CNY", "INR", "CAD", "AUD"};
    }
    if (key == "numberingSystem") {
        return {"arab", "arabext", "bali", "beng", "deva", "fullwide",
                "gujr", "guru", "hanidec", "khmr", "latn", "thai"};
    }
    if (key == "timeZone") {
        return {"UTC", "America/New_York", "America/Los_Angeles", 
                "Europe/London", "Europe/Paris", "Asia/Tokyo"};
    }
    if (key == "unit") {
        return {"acre", "bit", "byte", "celsius", "centimeter", "day",
                "degree", "fahrenheit", "foot", "gallon", "gram", "hour",
                "inch", "kilogram", "kilometer", "liter", "meter", "mile",
                "minute", "month", "ounce", "percent", "pound", "second",
                "week", "yard", "year"};
    }
    return {};
}

// ============================================================================
// IntlUtils
// ============================================================================

namespace IntlUtils {

std::string defaultLocale() {
    // Try to detect from environment
    const char* lang = std::getenv("LANG");
    if (lang) {
        std::string langStr(lang);
        size_t pos = langStr.find('.');
        if (pos != std::string::npos) {
            langStr = langStr.substr(0, pos);
        }
        std::replace(langStr.begin(), langStr.end(), '_', '-');
        return langStr;
    }
    return "en-US";
}

const std::vector<std::string>& availableLocales() {
    static std::vector<std::string> locales = {
        "ar", "ar-SA", "ar-EG",
        "de", "de-DE", "de-AT", "de-CH",
        "en", "en-US", "en-GB", "en-AU", "en-CA",
        "es", "es-ES", "es-MX", "es-AR",
        "fr", "fr-FR", "fr-CA", "fr-CH",
        "hi", "hi-IN",
        "it", "it-IT",
        "ja", "ja-JP",
        "ko", "ko-KR",
        "nl", "nl-NL", "nl-BE",
        "pl", "pl-PL",
        "pt", "pt-BR", "pt-PT",
        "ru", "ru-RU",
        "zh", "zh-CN", "zh-TW", "zh-HK"
    };
    return locales;
}

std::string bestFitMatcher(const std::vector<std::string>& requestedLocales,
                           const std::vector<std::string>& availableLocales) {
    for (const auto& req : requestedLocales) {
        // Exact match
        for (const auto& avail : availableLocales) {
            if (req == avail) return avail;
        }
        
        // Language match
        Locale parsed = Locale::parse(req);
        for (const auto& avail : availableLocales) {
            Locale availParsed = Locale::parse(avail);
            if (parsed.language == availParsed.language) {
                return avail;
            }
        }
    }
    
    return defaultLocale();
}

std::string lookupMatcher(const std::vector<std::string>& requestedLocales,
                          const std::vector<std::string>& availableLocales) {
    return bestFitMatcher(requestedLocales, availableLocales);
}

const CurrencyInfo& getCurrencyInfo(const std::string& currency) {
    static std::map<std::string, CurrencyInfo> currencies = {
        {"USD", {"$", "$", "US Dollar", 2}},
        {"EUR", {"€", "€", "Euro", 2}},
        {"GBP", {"£", "£", "British Pound", 2}},
        {"JPY", {"¥", "¥", "Japanese Yen", 0}},
        {"CNY", {"¥", "¥", "Chinese Yuan", 2}},
        {"INR", {"₹", "₹", "Indian Rupee", 2}},
        {"CAD", {"CA$", "$", "Canadian Dollar", 2}},
        {"AUD", {"A$", "$", "Australian Dollar", 2}},
        {"CHF", {"CHF", "CHF", "Swiss Franc", 2}},
        {"KRW", {"₩", "₩", "South Korean Won", 0}}
    };
    
    auto it = currencies.find(currency);
    if (it != currencies.end()) {
        return it->second;
    }
    
    static CurrencyInfo unknown = {"?", "?", "Unknown", 2};
    return unknown;
}

const UnitInfo& getUnitInfo(const std::string& unit) {
    static std::map<std::string, UnitInfo> units = {
        {"meter", {"meters", "m", "m"}},
        {"kilometer", {"kilometers", "km", "km"}},
        {"mile", {"miles", "mi", "mi"}},
        {"foot", {"feet", "ft", "ft"}},
        {"inch", {"inches", "in", "in"}},
        {"yard", {"yards", "yd", "yd"}},
        {"kilogram", {"kilograms", "kg", "kg"}},
        {"gram", {"grams", "g", "g"}},
        {"pound", {"pounds", "lb", "lb"}},
        {"ounce", {"ounces", "oz", "oz"}},
        {"liter", {"liters", "L", "L"}},
        {"gallon", {"gallons", "gal", "gal"}},
        {"celsius", {"degrees Celsius", "°C", "°"}},
        {"fahrenheit", {"degrees Fahrenheit", "°F", "°"}},
        {"second", {"seconds", "sec", "s"}},
        {"minute", {"minutes", "min", "m"}},
        {"hour", {"hours", "hr", "h"}},
        {"day", {"days", "day", "d"}},
        {"week", {"weeks", "wk", "w"}},
        {"month", {"months", "mo", "mo"}},
        {"year", {"years", "yr", "y"}},
        {"byte", {"bytes", "byte", "B"}},
        {"kilobyte", {"kilobytes", "kB", "kB"}},
        {"megabyte", {"megabytes", "MB", "MB"}},
        {"gigabyte", {"gigabytes", "GB", "GB"}},
        {"percent", {"percent", "%", "%"}}
    };
    
    auto it = units.find(unit);
    if (it != units.end()) {
        return it->second;
    }
    
    static UnitInfo unknown = {"", "", ""};
    return unknown;
}

} // namespace IntlUtils

// ============================================================================
// IntlSegmenter
// ============================================================================

IntlSegmenter::IntlSegmenter(const std::vector<std::string>& locales,
                             const std::map<std::string, Value>& options)
    : Object(ObjectType::Ordinary)
{
    std::string resolved = locales.empty() ?
        IntlUtils::defaultLocale() :
        IntlUtils::bestFitMatcher(locales, IntlUtils::availableLocales());
    
    locale_ = Locale::parse(resolved);
    
    auto it = options.find("granularity");
    if (it != options.end() && it->second.isString()) {
        granularity_ = it->second.asString()->value();
    }
}

std::vector<std::string> IntlSegmenter::segment(const std::string& text) const {
    std::vector<std::string> segments;
    
    if (granularity_ == "grapheme") {
        // Simple grapheme segmentation (byte-level for ASCII)
        for (size_t i = 0; i < text.length(); ) {
            unsigned char c = text[i];
            size_t len = 1;
            // UTF-8 multi-byte detection
            if ((c & 0x80) == 0) len = 1;
            else if ((c & 0xE0) == 0xC0) len = 2;
            else if ((c & 0xF0) == 0xE0) len = 3;
            else if ((c & 0xF8) == 0xF0) len = 4;
            else len = 1;
            
            if (i + len <= text.length()) {
                segments.push_back(text.substr(i, len));
            }
            i += len;
        }
    } else if (granularity_ == "word") {
        // Simple word segmentation
        std::string current;
        for (char c : text) {
            if (std::isalnum(c) || c == '\'') {
                current += c;
            } else {
                if (!current.empty()) {
                    segments.push_back(current);
                    current.clear();
                }
                if (!std::isspace(c)) {
                    segments.push_back(std::string(1, c));
                }
            }
        }
        if (!current.empty()) {
            segments.push_back(current);
        }
    } else if (granularity_ == "sentence") {
        // Simple sentence segmentation
        std::string current;
        for (size_t i = 0; i < text.length(); i++) {
            current += text[i];
            if (text[i] == '.' || text[i] == '!' || text[i] == '?') {
                // Check for end of sentence
                size_t next = i + 1;
                while (next < text.length() && std::isspace(text[next])) {
                    current += text[next];
                    next++;
                }
                segments.push_back(current);
                current.clear();
                i = next - 1;
            }
        }
        if (!current.empty()) {
            segments.push_back(current);
        }
    }
    
    return segments;
}

std::vector<std::string> IntlSegmenter::supportedLocalesOf(
    const std::vector<std::string>& locales) {
    return IntlCollator::supportedLocalesOf(locales);
}

// ============================================================================
// IntlDisplayNames
// ============================================================================

IntlDisplayNames::IntlDisplayNames(const std::vector<std::string>& locales,
                                   const std::map<std::string, Value>& options)
    : Object(ObjectType::Ordinary)
{
    std::string resolved = locales.empty() ?
        IntlUtils::defaultLocale() :
        IntlUtils::bestFitMatcher(locales, IntlUtils::availableLocales());
    
    locale_ = Locale::parse(resolved);
    
    auto it = options.find("type");
    if (it != options.end() && it->second.isString()) {
        type_ = it->second.asString()->value();
    }
    
    it = options.find("style");
    if (it != options.end() && it->second.isString()) {
        style_ = it->second.asString()->value();
    }
    
    it = options.find("fallback");
    if (it != options.end() && it->second.isString()) {
        fallback_ = it->second.asString()->value();
    }
}

std::string IntlDisplayNames::of(const std::string& code) const {
    // Language display names
    static std::map<std::string, std::string> languages = {
        {"en", "English"}, {"de", "German"}, {"fr", "French"},
        {"es", "Spanish"}, {"it", "Italian"}, {"pt", "Portuguese"},
        {"ru", "Russian"}, {"zh", "Chinese"}, {"ja", "Japanese"},
        {"ko", "Korean"}, {"ar", "Arabic"}, {"hi", "Hindi"},
        {"nl", "Dutch"}, {"pl", "Polish"}, {"tr", "Turkish"}
    };
    
    // Region display names
    static std::map<std::string, std::string> regions = {
        {"US", "United States"}, {"GB", "United Kingdom"},
        {"DE", "Germany"}, {"FR", "France"}, {"ES", "Spain"},
        {"IT", "Italy"}, {"JP", "Japan"}, {"CN", "China"},
        {"KR", "South Korea"}, {"BR", "Brazil"}, {"MX", "Mexico"},
        {"CA", "Canada"}, {"AU", "Australia"}, {"IN", "India"}
    };
    
    // Script display names
    static std::map<std::string, std::string> scripts = {
        {"Latn", "Latin"}, {"Cyrl", "Cyrillic"}, {"Arab", "Arabic"},
        {"Hans", "Simplified Chinese"}, {"Hant", "Traditional Chinese"},
        {"Jpan", "Japanese"}, {"Kore", "Korean"}, {"Deva", "Devanagari"}
    };
    
    // Currency display names
    static std::map<std::string, std::string> currencies = {
        {"USD", "US Dollar"}, {"EUR", "Euro"}, {"GBP", "British Pound"},
        {"JPY", "Japanese Yen"}, {"CNY", "Chinese Yuan"},
        {"INR", "Indian Rupee"}, {"CAD", "Canadian Dollar"}
    };
    
    std::map<std::string, std::string>* lookup = nullptr;
    
    if (type_ == "language") lookup = &languages;
    else if (type_ == "region") lookup = &regions;
    else if (type_ == "script") lookup = &scripts;
    else if (type_ == "currency") lookup = &currencies;
    
    if (lookup) {
        auto it = lookup->find(code);
        if (it != lookup->end()) {
            return it->second;
        }
    }
    
    // Fallback
    if (fallback_ == "code") {
        return code;
    }
    return "";
}

std::vector<std::string> IntlDisplayNames::supportedLocalesOf(
    const std::vector<std::string>& locales) {
    return IntlCollator::supportedLocalesOf(locales);
}

// ============================================================================
// IntlLocale
// ============================================================================

IntlLocale::IntlLocale(const std::string& tag,
                       const std::map<std::string, Value>& options)
    : Object(ObjectType::Ordinary)
{
    locale_ = Locale::parse(tag);
    
    // Apply options overrides
    auto it = options.find("calendar");
    if (it != options.end() && it->second.isString()) {
        locale_.calendar = it->second.asString()->value();
    }
    
    it = options.find("collation");
    if (it != options.end() && it->second.isString()) {
        locale_.collation = it->second.asString()->value();
    }
    
    it = options.find("numberingSystem");
    if (it != options.end() && it->second.isString()) {
        locale_.numberingSystem = it->second.asString()->value();
    }
    
    it = options.find("language");
    if (it != options.end() && it->second.isString()) {
        locale_.language = it->second.asString()->value();
    }
    
    it = options.find("script");
    if (it != options.end() && it->second.isString()) {
        locale_.script = it->second.asString()->value();
    }
    
    it = options.find("region");
    if (it != options.end() && it->second.isString()) {
        locale_.region = it->second.asString()->value();
    }
}

std::string IntlLocale::baseName() const {
    std::string result = locale_.language;
    if (!locale_.script.empty()) {
        result += "-" + locale_.script;
    }
    if (!locale_.region.empty()) {
        result += "-" + locale_.region;
    }
    return result;
}

IntlLocale* IntlLocale::maximize() const {
    // Add likely subtags
    std::string tag = locale_.toString();
    
    // Simple maximize rules
    Locale maximized = locale_;
    if (maximized.language == "en" && maximized.region.empty()) {
        maximized.region = "US";
        maximized.script = "Latn";
    } else if (maximized.language == "zh" && maximized.script.empty()) {
        maximized.script = "Hans";
        if (maximized.region.empty()) maximized.region = "CN";
    } else if (maximized.language == "ja" && maximized.region.empty()) {
        maximized.region = "JP";
        maximized.script = "Jpan";
    }
    
    auto* result = new IntlLocale(maximized.toString());
    return result;
}

IntlLocale* IntlLocale::minimize() const {
    // Remove unnecessary subtags
    Locale minimized = locale_;
    
    // Simple minimize: remove default script/region
    if (minimized.language == "en" && minimized.script == "Latn") {
        minimized.script.clear();
    }
    if (minimized.language == "en" && minimized.region == "US") {
        minimized.region.clear();
    }
    
    auto* result = new IntlLocale(minimized.toString());
    return result;
}

} // namespace Zepra::Runtime
