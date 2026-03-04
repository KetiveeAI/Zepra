/**
 * @file intl.hpp
 * @brief Internationalization API (ECMA-402)
 * 
 * Provides locale-sensitive string comparison, number formatting,
 * date/time formatting, and other i18n features.
 */

#pragma once

#include "runtime/objects/object.hpp"
#include "runtime/objects/value.hpp"
#include <string>
#include <vector>
#include <memory>
#include <locale>
#include <map>

namespace Zepra::Runtime {

class Function;

// Forward declarations
class IntlCollator;
class IntlNumberFormat;
class IntlDateTimeFormat;
class IntlPluralRules;
class IntlListFormat;
class IntlRelativeTimeFormat;
class IntlSegmenter;
class IntlDisplayNames;
class IntlLocale;

/**
 * @brief Locale identifier with parsed components
 */
struct Locale {
    std::string language;       // "en", "de", "ja"
    std::string script;         // "Latn", "Hans"
    std::string region;         // "US", "GB", "DE"
    std::string calendar;       // "gregory", "japanese"
    std::string collation;      // "standard", "phonebook"
    std::string numberingSystem;// "latn", "arab"
    
    std::string toString() const;
    static Locale parse(const std::string& tag);
    static std::vector<std::string> getCanonicalLocales(const std::vector<std::string>& locales);
};

/**
 * @brief Options for collator
 */
struct CollatorOptions {
    std::string usage = "sort";          // "sort" or "search"
    std::string sensitivity = "variant"; // "base", "accent", "case", "variant"
    bool ignorePunctuation = false;
    bool numeric = false;
    std::string caseFirst = "false";     // "upper", "lower", "false"
    std::string collation = "default";
};

/**
 * @brief Intl.Collator - Locale-sensitive string comparison
 */
class IntlCollator : public Object {
public:
    IntlCollator(const std::vector<std::string>& locales, const CollatorOptions& options);
    
    // Compare two strings
    int compare(const std::string& x, const std::string& y) const;
    
    // Get resolved options
    Object* resolvedOptions() const;
    
    // Static methods
    static std::vector<std::string> supportedLocalesOf(
        const std::vector<std::string>& locales);
    
private:
    Locale locale_;
    CollatorOptions options_;
    std::locale cppLocale_;
};

/**
 * @brief Options for number formatting
 */
struct NumberFormatOptions {
    std::string style = "decimal";       // "decimal", "currency", "percent", "unit"
    std::string currency;                // "USD", "EUR"
    std::string currencyDisplay = "symbol"; // "symbol", "narrowSymbol", "code", "name"
    std::string currencySign = "standard";  // "standard", "accounting"
    std::string unit;                    // "mile", "kilometer"
    std::string unitDisplay = "short";   // "long", "short", "narrow"
    std::string notation = "standard";   // "standard", "scientific", "engineering", "compact"
    std::string compactDisplay = "short";
    std::string signDisplay = "auto";    // "auto", "never", "always", "exceptZero"
    int minimumIntegerDigits = 1;
    int minimumFractionDigits = -1;      // -1 = use default
    int maximumFractionDigits = -1;
    int minimumSignificantDigits = -1;
    int maximumSignificantDigits = -1;
    std::string roundingMode = "halfExpand";
    bool useGrouping = true;
};

/**
 * @brief Intl.NumberFormat - Locale-sensitive number formatting
 */
class IntlNumberFormat : public Object {
public:
    IntlNumberFormat(const std::vector<std::string>& locales, 
                     const NumberFormatOptions& options);
    
    // Format a number to string
    std::string format(double value) const;
    
    // Format to parts
    std::vector<std::pair<std::string, std::string>> formatToParts(double value) const;
    
    // Format range
    std::string formatRange(double start, double end) const;
    
    // Get resolved options
    Object* resolvedOptions() const;
    
    // Static methods
    static std::vector<std::string> supportedLocalesOf(
        const std::vector<std::string>& locales);
    
private:
    Locale locale_;
    NumberFormatOptions options_;
    
    std::string formatDecimal(double value) const;
    std::string formatCurrency(double value) const;
    std::string formatPercent(double value) const;
    std::string formatUnit(double value) const;
};

/**
 * @brief Options for date/time formatting
 */
struct DateTimeFormatOptions {
    std::string dateStyle;           // "full", "long", "medium", "short"
    std::string timeStyle;           // "full", "long", "medium", "short"
    std::string calendar = "gregory";
    std::string numberingSystem = "latn";
    std::string timeZone;
    bool hour12;
    std::string hourCycle;           // "h11", "h12", "h23", "h24"
    std::string weekday;             // "narrow", "short", "long"
    std::string era;
    std::string year;                // "numeric", "2-digit"
    std::string month;               // "numeric", "2-digit", "narrow", "short", "long"
    std::string day;
    std::string dayPeriod;
    std::string hour;
    std::string minute;
    std::string second;
    std::string fractionalSecondDigits;
    std::string timeZoneName;
};

/**
 * @brief Intl.DateTimeFormat - Locale-sensitive date/time formatting
 */
class IntlDateTimeFormat : public Object {
public:
    IntlDateTimeFormat(const std::vector<std::string>& locales,
                       const DateTimeFormatOptions& options);
    
    // Format a date
    std::string format(double dateValue) const;
    
    // Format to parts
    std::vector<std::pair<std::string, std::string>> formatToParts(double dateValue) const;
    
    // Format range
    std::string formatRange(double startDate, double endDate) const;
    
    // Get resolved options
    Object* resolvedOptions() const;
    
    static std::vector<std::string> supportedLocalesOf(
        const std::vector<std::string>& locales);
    
private:
    Locale locale_;
    DateTimeFormatOptions options_;
};

/**
 * @brief Intl.PluralRules - Language-sensitive plural formatting
 */
class IntlPluralRules : public Object {
public:
    IntlPluralRules(const std::vector<std::string>& locales,
                    const std::map<std::string, Value>& options);
    
    // Get plural category: "zero", "one", "two", "few", "many", "other"
    std::string select(double n) const;
    
    // Get plural categories in use
    std::vector<std::string> resolvedOptions() const;
    
    static std::vector<std::string> supportedLocalesOf(
        const std::vector<std::string>& locales);
    
private:
    Locale locale_;
    std::string type_ = "cardinal"; // "cardinal" or "ordinal"
};

/**
 * @brief Intl.ListFormat - Language-sensitive list formatting
 */
class IntlListFormat : public Object {
public:
    IntlListFormat(const std::vector<std::string>& locales,
                   const std::map<std::string, Value>& options);
    
    // Format a list: ["a", "b", "c"] -> "a, b, and c"
    std::string format(const std::vector<std::string>& list) const;
    
    // Format to parts
    std::vector<std::pair<std::string, std::string>> formatToParts(
        const std::vector<std::string>& list) const;
    
    static std::vector<std::string> supportedLocalesOf(
        const std::vector<std::string>& locales);
    
private:
    Locale locale_;
    std::string type_ = "conjunction";  // "conjunction", "disjunction", "unit"
    std::string style_ = "long";        // "long", "short", "narrow"
};

/**
 * @brief Intl.RelativeTimeFormat - Relative time formatting
 */
class IntlRelativeTimeFormat : public Object {
public:
    IntlRelativeTimeFormat(const std::vector<std::string>& locales,
                           const std::map<std::string, Value>& options);
    
    // Format relative time: (-1, "day") -> "yesterday"
    std::string format(double value, const std::string& unit) const;
    
    // Format to parts
    std::vector<std::pair<std::string, std::string>> formatToParts(
        double value, const std::string& unit) const;
    
    static std::vector<std::string> supportedLocalesOf(
        const std::vector<std::string>& locales);
    
private:
    Locale locale_;
    std::string style_ = "long";    // "long", "short", "narrow"
    std::string numeric_ = "always"; // "always", "auto"
};

/**
 * @brief Intl.Segmenter - Text segmentation
 */
class IntlSegmenter : public Object {
public:
    IntlSegmenter(const std::vector<std::string>& locales,
                  const std::map<std::string, Value>& options);
    
    // Segment text into graphemes, words, or sentences
    std::vector<std::string> segment(const std::string& text) const;
    
    static std::vector<std::string> supportedLocalesOf(
        const std::vector<std::string>& locales);
    
private:
    Locale locale_;
    std::string granularity_ = "grapheme"; // "grapheme", "word", "sentence"
};

/**
 * @brief Intl.DisplayNames - Display names for languages, regions, etc.
 */
class IntlDisplayNames : public Object {
public:
    IntlDisplayNames(const std::vector<std::string>& locales,
                     const std::map<std::string, Value>& options);
    
    // Get display name for a code
    std::string of(const std::string& code) const;
    
    static std::vector<std::string> supportedLocalesOf(
        const std::vector<std::string>& locales);
    
private:
    Locale locale_;
    std::string type_;      // "language", "region", "script", "currency", "calendar", "dateTimeField"
    std::string style_ = "long";
    std::string fallback_ = "code";
    std::string languageDisplay_ = "dialect";
};

/**
 * @brief Intl.Locale - Locale data and manipulation
 */
class IntlLocale : public Object {
public:
    explicit IntlLocale(const std::string& tag,
                        const std::map<std::string, Value>& options = {});
    
    // Properties
    std::string baseName() const;
    std::string language() const { return locale_.language; }
    std::string script() const { return locale_.script; }
    std::string region() const { return locale_.region; }
    std::string calendar() const { return locale_.calendar; }
    std::string collation() const { return locale_.collation; }
    std::string numberingSystem() const { return locale_.numberingSystem; }
    
    // Methods
    IntlLocale* maximize() const;
    IntlLocale* minimize() const;
    std::string toString() const { return locale_.toString(); }
    
private:
    Locale locale_;
};

/**
 * @brief Intl namespace object
 */
class IntlObject : public Object {
public:
    IntlObject();
    
    // Intl.getCanonicalLocales(locales)
    static std::vector<std::string> getCanonicalLocales(
        const std::vector<std::string>& locales);
    
    // Intl.supportedValuesOf(key)
    static std::vector<std::string> supportedValuesOf(const std::string& key);
};

/**
 * @brief Utility functions for Intl
 */
namespace IntlUtils {

// Best fit locale matching
std::string bestFitMatcher(const std::vector<std::string>& requestedLocales,
                           const std::vector<std::string>& availableLocales);

// Lookup locale matching
std::string lookupMatcher(const std::vector<std::string>& requestedLocales,
                          const std::vector<std::string>& availableLocales);

// Default locale
std::string defaultLocale();

// Supported locales (built-in)
const std::vector<std::string>& availableLocales();

// Currency data
struct CurrencyInfo {
    std::string symbol;
    std::string narrowSymbol;
    std::string name;
    int digits;
};
const CurrencyInfo& getCurrencyInfo(const std::string& currency);

// Unit data
struct UnitInfo {
    std::string longName;
    std::string shortName;
    std::string narrowName;
};
const UnitInfo& getUnitInfo(const std::string& unit);

} // namespace IntlUtils

} // namespace Zepra::Runtime
