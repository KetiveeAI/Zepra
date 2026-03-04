/**
 * @file IntlCollatorAPI.h
 * @brief Intl.Collator Implementation
 */

#pragma once

#include <string>
#include <vector>
#include <algorithm>

namespace Zepra::Runtime {

// =============================================================================
// Collator Options
// =============================================================================

enum class CollatorUsage { Sort, Search };
enum class CollatorSensitivity { Base, Accent, Case, Variant };
enum class CaseFirst { Upper, Lower, False };

struct CollatorOptions {
    std::string locale = "en";
    CollatorUsage usage = CollatorUsage::Sort;
    CollatorSensitivity sensitivity = CollatorSensitivity::Variant;
    bool ignorePunctuation = false;
    bool numeric = false;
    CaseFirst caseFirst = CaseFirst::False;
};

// =============================================================================
// Intl.Collator
// =============================================================================

class Collator {
public:
    Collator(const std::string& locale = "en", CollatorOptions options = {})
        : locale_(locale), options_(std::move(options)) {}
    
    int compare(const std::string& a, const std::string& b) const {
        std::string strA = transform(a);
        std::string strB = transform(b);
        
        if (options_.numeric) {
            return numericCompare(strA, strB);
        }
        
        if (strA < strB) return -1;
        if (strA > strB) return 1;
        return 0;
    }
    
    struct ResolvedOptions {
        std::string locale;
        std::string usage;
        std::string sensitivity;
        bool ignorePunctuation;
        bool numeric;
        std::string caseFirst;
        std::string collation;
    };
    
    ResolvedOptions resolvedOptions() const {
        ResolvedOptions opts;
        opts.locale = locale_;
        opts.collation = "default";
        opts.ignorePunctuation = options_.ignorePunctuation;
        opts.numeric = options_.numeric;
        
        switch (options_.usage) {
            case CollatorUsage::Sort: opts.usage = "sort"; break;
            case CollatorUsage::Search: opts.usage = "search"; break;
        }
        
        switch (options_.sensitivity) {
            case CollatorSensitivity::Base: opts.sensitivity = "base"; break;
            case CollatorSensitivity::Accent: opts.sensitivity = "accent"; break;
            case CollatorSensitivity::Case: opts.sensitivity = "case"; break;
            case CollatorSensitivity::Variant: opts.sensitivity = "variant"; break;
        }
        
        switch (options_.caseFirst) {
            case CaseFirst::Upper: opts.caseFirst = "upper"; break;
            case CaseFirst::Lower: opts.caseFirst = "lower"; break;
            case CaseFirst::False: opts.caseFirst = "false"; break;
        }
        
        return opts;
    }
    
    template<typename Container>
    void sort(Container& items) const {
        std::sort(items.begin(), items.end(),
            [this](const std::string& a, const std::string& b) {
                return compare(a, b) < 0;
            });
    }
    
    static std::vector<std::string> supportedLocales() {
        return {"en", "en-US", "es", "fr", "de", "ja", "zh"};
    }

private:
    std::string transform(const std::string& str) const {
        std::string result = str;
        
        switch (options_.sensitivity) {
            case CollatorSensitivity::Base:
                result = toLowerCase(result);
                result = removeAccents(result);
                break;
            case CollatorSensitivity::Accent:
                result = toLowerCase(result);
                break;
            case CollatorSensitivity::Case:
                result = removeAccents(result);
                break;
            case CollatorSensitivity::Variant:
                break;
        }
        
        if (options_.ignorePunctuation) {
            result = removePunctuation(result);
        }
        
        return result;
    }
    
    std::string toLowerCase(const std::string& str) const {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }
    
    std::string removeAccents(const std::string& str) const {
        // Simplified ASCII-only implementation
        return str;
    }
    
    std::string removePunctuation(const std::string& str) const {
        std::string result;
        for (char c : str) {
            if (!std::ispunct(c)) result += c;
        }
        return result;
    }
    
    int numericCompare(const std::string& a, const std::string& b) const {
        size_t i = 0, j = 0;
        
        while (i < a.length() && j < b.length()) {
            if (std::isdigit(a[i]) && std::isdigit(b[j])) {
                size_t numStartA = i, numStartB = j;
                while (i < a.length() && std::isdigit(a[i])) ++i;
                while (j < b.length() && std::isdigit(b[j])) ++j;
                
                long numA = std::stol(a.substr(numStartA, i - numStartA));
                long numB = std::stol(b.substr(numStartB, j - numStartB));
                
                if (numA != numB) return numA < numB ? -1 : 1;
            } else {
                if (a[i] != b[j]) return a[i] < b[j] ? -1 : 1;
                ++i;
                ++j;
            }
        }
        
        if (i < a.length()) return 1;
        if (j < b.length()) return -1;
        return 0;
    }
    
    std::string locale_;
    CollatorOptions options_;
};

} // namespace Zepra::Runtime
