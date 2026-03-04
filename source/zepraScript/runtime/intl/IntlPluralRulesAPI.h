/**
 * @file IntlPluralRulesAPI.h
 * @brief Intl.PluralRules Implementation
 */

#pragma once

#include <string>
#include <vector>
#include <cmath>

namespace Zepra::Runtime {

// =============================================================================
// PluralRules Type
// =============================================================================

enum class PluralRulesType { Cardinal, Ordinal };

// =============================================================================
// PluralRules Options
// =============================================================================

struct PluralRulesOptions {
    std::string locale = "en";
    PluralRulesType type = PluralRulesType::Cardinal;
    int minimumIntegerDigits = 1;
    int minimumFractionDigits = 0;
    int maximumFractionDigits = 3;
};

// =============================================================================
// Intl.PluralRules
// =============================================================================

class PluralRules {
public:
    PluralRules(const std::string& locale = "en", PluralRulesOptions options = {})
        : locale_(locale), options_(std::move(options)) {}
    
    std::string select(double n) const {
        if (options_.type == PluralRulesType::Ordinal) {
            return selectOrdinal(n);
        }
        return selectCardinal(n);
    }
    
    std::vector<std::string> resolvedCategories() const {
        if (options_.type == PluralRulesType::Ordinal) {
            return {"one", "two", "few", "other"};
        }
        return {"one", "other"};
    }
    
    struct ResolvedOptions {
        std::string locale;
        std::string type;
        int minimumIntegerDigits;
        int minimumFractionDigits;
        int maximumFractionDigits;
        std::string pluralCategories;
    };
    
    ResolvedOptions resolvedOptions() const {
        ResolvedOptions opts;
        opts.locale = locale_;
        opts.type = (options_.type == PluralRulesType::Cardinal) ? "cardinal" : "ordinal";
        opts.minimumIntegerDigits = options_.minimumIntegerDigits;
        opts.minimumFractionDigits = options_.minimumFractionDigits;
        opts.maximumFractionDigits = options_.maximumFractionDigits;
        return opts;
    }
    
    static std::vector<std::string> supportedLocales() {
        return {"en", "en-US", "es", "fr", "de", "ja", "zh", "ru", "ar"};
    }

private:
    std::string selectCardinal(double n) const {
        // English cardinal rules
        if (locale_.find("en") == 0) {
            int i = static_cast<int>(std::abs(n));
            if (i == 1 && n == std::floor(n)) return "one";
            return "other";
        }
        
        // French
        if (locale_.find("fr") == 0) {
            if (n >= 0 && n < 2) return "one";
            return "other";
        }
        
        // Russian (simplified)
        if (locale_.find("ru") == 0) {
            int i = static_cast<int>(std::abs(n)) % 100;
            int i10 = i % 10;
            if (i10 == 1 && i != 11) return "one";
            if (i10 >= 2 && i10 <= 4 && (i < 12 || i > 14)) return "few";
            return "other";
        }
        
        // Arabic (simplified)
        if (locale_.find("ar") == 0) {
            int i = static_cast<int>(std::abs(n));
            if (i == 0) return "zero";
            if (i == 1) return "one";
            if (i == 2) return "two";
            int i100 = i % 100;
            if (i100 >= 3 && i100 <= 10) return "few";
            if (i100 >= 11 && i100 <= 99) return "many";
            return "other";
        }
        
        // Default (one/other)
        if (n == 1) return "one";
        return "other";
    }
    
    std::string selectOrdinal(double n) const {
        // English ordinal rules
        if (locale_.find("en") == 0) {
            int i = static_cast<int>(std::abs(n)) % 100;
            int i10 = static_cast<int>(std::abs(n)) % 10;
            
            if (i10 == 1 && i != 11) return "one";   // 1st, 21st, 31st
            if (i10 == 2 && i != 12) return "two";   // 2nd, 22nd, 32nd
            if (i10 == 3 && i != 13) return "few";   // 3rd, 23rd, 33rd
            return "other";  // 4th, 11th, 12th, 13th, etc.
        }
        
        return "other";
    }
    
    std::string locale_;
    PluralRulesOptions options_;
};

} // namespace Zepra::Runtime
