/**
 * @file IntlAPI.h
 * @brief Internationalization API Implementation
 * 
 * ECMA-402 Internationalization API:
 * - Intl.DateTimeFormat
 * - Intl.NumberFormat
 * - Intl.Collator
 */

#pragma once

#include <string>
#include <vector>
#include <optional>
#include <chrono>
#include <ctime>
#include <cmath>

namespace Zepra::API {
namespace Intl {

// =============================================================================
// Locale
// =============================================================================

/**
 * @brief Locale representation
 */
class Locale {
public:
    explicit Locale(const std::string& tag = "en-US")
        : baseName_(tag) {
        parse(tag);
    }
    
    const std::string& baseName() const { return baseName_; }
    const std::string& language() const { return language_; }
    const std::string& region() const { return region_; }
    const std::string& script() const { return script_; }
    
    std::string toString() const { return baseName_; }
    
private:
    void parse(const std::string& tag) {
        size_t pos = 0;
        size_t dash = tag.find('-');
        
        if (dash != std::string::npos) {
            language_ = tag.substr(0, dash);
            pos = dash + 1;
            
            if (pos < tag.length()) {
                size_t next = tag.find('-', pos);
                std::string part = tag.substr(pos, next - pos);
                
                if (part.length() == 4) {
                    script_ = part;
                    pos = next != std::string::npos ? next + 1 : tag.length();
                }
                
                if (pos < tag.length()) {
                    region_ = tag.substr(pos);
                }
            }
        } else {
            language_ = tag;
        }
    }
    
    std::string baseName_;
    std::string language_;
    std::string region_;
    std::string script_;
};

// =============================================================================
// DateTimeFormat Options
// =============================================================================

struct DateTimeFormatOptions {
    std::optional<std::string> dateStyle;   // "full", "long", "medium", "short"
    std::optional<std::string> timeStyle;   // "full", "long", "medium", "short"
    std::optional<std::string> weekday;     // "long", "short", "narrow"
    std::optional<std::string> year;        // "numeric", "2-digit"
    std::optional<std::string> month;       // "numeric", "2-digit", "long", "short", "narrow"
    std::optional<std::string> day;         // "numeric", "2-digit"
    std::optional<std::string> hour;        // "numeric", "2-digit"
    std::optional<std::string> minute;      // "numeric", "2-digit"
    std::optional<std::string> second;      // "numeric", "2-digit"
    std::optional<std::string> timeZone;
    bool hour12 = false;
};

// =============================================================================
// DateTimeFormat
// =============================================================================

/**
 * @brief Locale-sensitive date/time formatting
 */
class DateTimeFormat {
public:
    DateTimeFormat(const std::string& locale = "en-US",
                   const DateTimeFormatOptions& options = {})
        : locale_(locale), options_(options) {}
    
    // Format date
    std::string format(double timestamp) const {
        std::time_t time = static_cast<std::time_t>(timestamp / 1000);
        std::tm* tm = std::localtime(&time);
        
        char buffer[128];
        
        if (options_.dateStyle) {
            if (*options_.dateStyle == "full") {
                std::strftime(buffer, sizeof(buffer), "%A, %B %d, %Y", tm);
            } else if (*options_.dateStyle == "long") {
                std::strftime(buffer, sizeof(buffer), "%B %d, %Y", tm);
            } else if (*options_.dateStyle == "medium") {
                std::strftime(buffer, sizeof(buffer), "%b %d, %Y", tm);
            } else {
                std::strftime(buffer, sizeof(buffer), "%m/%d/%y", tm);
            }
        } else if (options_.timeStyle) {
            if (*options_.timeStyle == "full" || *options_.timeStyle == "long") {
                std::strftime(buffer, sizeof(buffer), "%H:%M:%S %Z", tm);
            } else if (*options_.timeStyle == "medium") {
                std::strftime(buffer, sizeof(buffer), "%H:%M:%S", tm);
            } else {
                std::strftime(buffer, sizeof(buffer), "%H:%M", tm);
            }
        } else {
            std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", tm);
        }
        
        return std::string(buffer);
    }
    
    // Format to parts
    struct FormatPart {
        std::string type;
        std::string value;
    };
    
    std::vector<FormatPart> formatToParts(double timestamp) const {
        std::vector<FormatPart> parts;
        std::time_t time = static_cast<std::time_t>(timestamp / 1000);
        std::tm* tm = std::localtime(&time);
        
        char buffer[32];
        
        if (options_.weekday) {
            std::strftime(buffer, sizeof(buffer), "%A", tm);
            parts.push_back({"weekday", buffer});
            parts.push_back({"literal", ", "});
        }
        
        std::strftime(buffer, sizeof(buffer), "%B", tm);
        parts.push_back({"month", buffer});
        parts.push_back({"literal", " "});
        
        std::strftime(buffer, sizeof(buffer), "%d", tm);
        parts.push_back({"day", buffer});
        parts.push_back({"literal", ", "});
        
        std::strftime(buffer, sizeof(buffer), "%Y", tm);
        parts.push_back({"year", buffer});
        
        return parts;
    }
    
private:
    Locale locale_;
    DateTimeFormatOptions options_;
};

// =============================================================================
// NumberFormat Options
// =============================================================================

struct NumberFormatOptions {
    std::string style = "decimal";  // "decimal", "currency", "percent", "unit"
    std::optional<std::string> currency;
    std::optional<std::string> currencyDisplay;  // "symbol", "code", "name"
    std::optional<std::string> unit;
    std::optional<int> minimumIntegerDigits;
    std::optional<int> minimumFractionDigits;
    std::optional<int> maximumFractionDigits;
    std::optional<int> minimumSignificantDigits;
    std::optional<int> maximumSignificantDigits;
    bool useGrouping = true;
};

// =============================================================================
// NumberFormat
// =============================================================================

/**
 * @brief Locale-sensitive number formatting
 */
class NumberFormat {
public:
    NumberFormat(const std::string& locale = "en-US",
                 const NumberFormatOptions& options = {})
        : locale_(locale), options_(options) {}
    
    // Format number
    std::string format(double value) const {
        if (options_.style == "currency") {
            return formatCurrency(value);
        } else if (options_.style == "percent") {
            return formatPercent(value);
        }
        return formatDecimal(value);
    }
    
private:
    std::string formatDecimal(double value) const {
        char buffer[64];
        int fracDigits = options_.maximumFractionDigits.value_or(2);
        snprintf(buffer, sizeof(buffer), "%.*f", fracDigits, value);
        
        std::string result = buffer;
        
        if (options_.useGrouping) {
            result = addGrouping(result);
        }
        
        return result;
    }
    
    std::string formatCurrency(double value) const {
        std::string num = formatDecimal(value);
        std::string symbol = "$";  // Would look up from currency code
        
        if (options_.currency) {
            if (*options_.currency == "EUR") symbol = "€";
            else if (*options_.currency == "GBP") symbol = "£";
            else if (*options_.currency == "JPY") symbol = "¥";
            else if (*options_.currency == "INR") symbol = "₹";
        }
        
        return symbol + num;
    }
    
    std::string formatPercent(double value) const {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%.0f%%", value * 100);
        return buffer;
    }
    
    std::string addGrouping(const std::string& num) const {
        std::string result;
        size_t dotPos = num.find('.');
        std::string intPart = num.substr(0, dotPos);
        std::string fracPart = dotPos != std::string::npos ? num.substr(dotPos) : "";
        
        int count = 0;
        for (int i = intPart.length() - 1; i >= 0; i--) {
            if (count > 0 && count % 3 == 0 && intPart[i] != '-') {
                result = "," + result;
            }
            result = intPart[i] + result;
            count++;
        }
        
        return result + fracPart;
    }
    
    Locale locale_;
    NumberFormatOptions options_;
};

// =============================================================================
// Collator Options
// =============================================================================

struct CollatorOptions {
    std::string usage = "sort";     // "sort", "search"
    std::string sensitivity = "variant";  // "base", "accent", "case", "variant"
    bool numeric = false;
    bool ignorePunctuation = false;
};

// =============================================================================
// Collator
// =============================================================================

/**
 * @brief Locale-sensitive string comparison
 */
class Collator {
public:
    Collator(const std::string& locale = "en-US",
             const CollatorOptions& options = {})
        : locale_(locale), options_(options) {}
    
    // Compare strings
    int compare(const std::string& a, const std::string& b) const {
        if (options_.numeric) {
            return compareNumeric(a, b);
        }
        
        if (options_.sensitivity == "base") {
            return compareBase(a, b);
        }
        
        return a.compare(b);
    }
    
private:
    int compareBase(const std::string& a, const std::string& b) const {
        std::string la, lb;
        for (char c : a) la += std::tolower(c);
        for (char c : b) lb += std::tolower(c);
        return la.compare(lb);
    }
    
    int compareNumeric(const std::string& a, const std::string& b) const {
        // Simple numeric comparison
        try {
            double na = std::stod(a);
            double nb = std::stod(b);
            if (na < nb) return -1;
            if (na > nb) return 1;
            return 0;
        } catch (...) {
            return a.compare(b);
        }
    }
    
    Locale locale_;
    CollatorOptions options_;
};

} // namespace Intl
} // namespace Zepra::API
