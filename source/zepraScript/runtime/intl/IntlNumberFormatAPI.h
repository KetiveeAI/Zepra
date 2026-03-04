/**
 * @file IntlNumberFormatAPI.h
 * @brief Intl.NumberFormat Implementation
 */

#pragma once

#include <string>
#include <vector>
#include <cmath>
#include <sstream>
#include <iomanip>

namespace Zepra::Runtime {

// =============================================================================
// NumberFormat Style
// =============================================================================

enum class NumberFormatStyle { Decimal, Currency, Percent, Unit };
enum class CurrencyDisplay { Symbol, NarrowSymbol, Code, Name };
enum class UnitDisplay { Short, Narrow, Long };
enum class Notation { Standard, Scientific, Engineering, Compact };
enum class SignDisplay { Auto, Never, Always, ExceptZero };

// =============================================================================
// NumberFormat Options
// =============================================================================

struct NumberFormatOptions {
    std::string locale = "en";
    NumberFormatStyle style = NumberFormatStyle::Decimal;
    std::string currency;
    CurrencyDisplay currencyDisplay = CurrencyDisplay::Symbol;
    std::string unit;
    UnitDisplay unitDisplay = UnitDisplay::Short;
    Notation notation = Notation::Standard;
    SignDisplay signDisplay = SignDisplay::Auto;
    int minimumIntegerDigits = 1;
    int minimumFractionDigits = 0;
    int maximumFractionDigits = 3;
    bool useGrouping = true;
};

// =============================================================================
// Intl.NumberFormat
// =============================================================================

class NumberFormat {
public:
    NumberFormat(const std::string& locale = "en", NumberFormatOptions options = {})
        : locale_(locale), options_(std::move(options)) {}
    
    std::string format(double value) const {
        std::ostringstream oss;
        
        // Handle sign
        bool negative = value < 0;
        double absValue = std::abs(value);
        
        switch (options_.style) {
            case NumberFormatStyle::Decimal:
                formatDecimal(oss, absValue);
                break;
            case NumberFormatStyle::Currency:
                formatCurrency(oss, absValue);
                break;
            case NumberFormatStyle::Percent:
                formatPercent(oss, absValue);
                break;
            case NumberFormatStyle::Unit:
                formatUnit(oss, absValue);
                break;
        }
        
        std::string result = oss.str();
        
        if (negative && shouldShowSign(value)) {
            if (options_.style == NumberFormatStyle::Currency) {
                result = "-" + result;
            } else {
                result = "-" + result;
            }
        } else if (options_.signDisplay == SignDisplay::Always && value > 0) {
            result = "+" + result;
        }
        
        return result;
    }
    
    struct FormatPart {
        std::string type;
        std::string value;
    };
    
    std::vector<FormatPart> formatToParts(double value) const {
        std::vector<FormatPart> parts;
        
        bool negative = value < 0;
        double absValue = std::abs(value);
        
        if (negative) {
            parts.push_back({"minusSign", "-"});
        }
        
        std::string formatted = formatDecimalRaw(absValue);
        
        size_t decimalPos = formatted.find('.');
        if (decimalPos != std::string::npos) {
            parts.push_back({"integer", formatted.substr(0, decimalPos)});
            parts.push_back({"decimal", "."});
            parts.push_back({"fraction", formatted.substr(decimalPos + 1)});
        } else {
            parts.push_back({"integer", formatted});
        }
        
        return parts;
    }
    
    struct ResolvedOptions {
        std::string locale;
        std::string numberingSystem;
        std::string style;
        std::string currency;
        std::string currencyDisplay;
        int minimumIntegerDigits;
        int minimumFractionDigits;
        int maximumFractionDigits;
        bool useGrouping;
    };
    
    ResolvedOptions resolvedOptions() const {
        ResolvedOptions opts;
        opts.locale = locale_;
        opts.numberingSystem = "latn";
        opts.minimumIntegerDigits = options_.minimumIntegerDigits;
        opts.minimumFractionDigits = options_.minimumFractionDigits;
        opts.maximumFractionDigits = options_.maximumFractionDigits;
        opts.useGrouping = options_.useGrouping;
        
        switch (options_.style) {
            case NumberFormatStyle::Decimal: opts.style = "decimal"; break;
            case NumberFormatStyle::Currency: opts.style = "currency"; break;
            case NumberFormatStyle::Percent: opts.style = "percent"; break;
            case NumberFormatStyle::Unit: opts.style = "unit"; break;
        }
        
        opts.currency = options_.currency;
        
        return opts;
    }
    
    static std::vector<std::string> supportedLocales() {
        return {"en", "en-US", "es", "fr", "de", "ja", "zh", "ko"};
    }

private:
    void formatDecimal(std::ostringstream& oss, double value) const {
        oss << std::fixed << std::setprecision(options_.maximumFractionDigits) << value;
        if (options_.useGrouping) {
            std::string str = oss.str();
            oss.str("");
            oss << addGrouping(str);
        }
    }
    
    void formatCurrency(std::ostringstream& oss, double value) const {
        std::string symbol = getCurrencySymbol(options_.currency);
        oss << symbol << std::fixed << std::setprecision(2) << value;
    }
    
    void formatPercent(std::ostringstream& oss, double value) const {
        oss << std::fixed << std::setprecision(0) << (value * 100) << "%";
    }
    
    void formatUnit(std::ostringstream& oss, double value) const {
        oss << std::fixed << std::setprecision(options_.maximumFractionDigits) << value;
        oss << " " << options_.unit;
    }
    
    std::string formatDecimalRaw(double value) const {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(options_.maximumFractionDigits) << value;
        return oss.str();
    }
    
    std::string addGrouping(const std::string& str) const {
        size_t decPos = str.find('.');
        std::string intPart = (decPos != std::string::npos) ? str.substr(0, decPos) : str;
        std::string fracPart = (decPos != std::string::npos) ? str.substr(decPos) : "";
        
        std::string result;
        int count = 0;
        for (auto it = intPart.rbegin(); it != intPart.rend(); ++it) {
            if (count > 0 && count % 3 == 0) result = ',' + result;
            result = *it + result;
            ++count;
        }
        
        return result + fracPart;
    }
    
    std::string getCurrencySymbol(const std::string& code) const {
        static const std::map<std::string, std::string> symbols = {
            {"USD", "$"}, {"EUR", "€"}, {"GBP", "£"}, {"JPY", "¥"},
            {"CNY", "¥"}, {"INR", "₹"}, {"KRW", "₩"}, {"RUB", "₽"}
        };
        auto it = symbols.find(code);
        return it != symbols.end() ? it->second : code;
    }
    
    bool shouldShowSign(double value) const {
        return options_.signDisplay != SignDisplay::Never;
    }
    
    std::string locale_;
    NumberFormatOptions options_;
};

} // namespace Zepra::Runtime
