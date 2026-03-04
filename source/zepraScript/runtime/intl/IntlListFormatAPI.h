/**
 * @file IntlListFormatAPI.h
 * @brief Intl.ListFormat Implementation
 */

#pragma once

#include <string>
#include <vector>

namespace Zepra::Runtime {

// =============================================================================
// ListFormat Type & Style
// =============================================================================

enum class ListFormatType { Conjunction, Disjunction, Unit };
enum class ListFormatStyle { Long, Short, Narrow };

// =============================================================================
// ListFormat Options
// =============================================================================

struct ListFormatOptions {
    std::string locale = "en";
    ListFormatType type = ListFormatType::Conjunction;
    ListFormatStyle style = ListFormatStyle::Long;
};

// =============================================================================
// Intl.ListFormat
// =============================================================================

class ListFormat {
public:
    ListFormat(const std::string& locale = "en", ListFormatOptions options = {})
        : locale_(locale), options_(std::move(options)) {}
    
    std::string format(const std::vector<std::string>& list) const {
        if (list.empty()) return "";
        if (list.size() == 1) return list[0];
        
        std::string connector = getConnector();
        std::string result;
        
        for (size_t i = 0; i < list.size(); ++i) {
            if (i > 0) {
                if (i == list.size() - 1) {
                    result += connector;
                } else {
                    result += getSeparator();
                }
            }
            result += list[i];
        }
        
        return result;
    }
    
    struct FormatPart {
        std::string type;
        std::string value;
    };
    
    std::vector<FormatPart> formatToParts(const std::vector<std::string>& list) const {
        std::vector<FormatPart> parts;
        
        for (size_t i = 0; i < list.size(); ++i) {
            if (i > 0) {
                if (i == list.size() - 1) {
                    parts.push_back({"literal", getConnector()});
                } else {
                    parts.push_back({"literal", getSeparator()});
                }
            }
            parts.push_back({"element", list[i]});
        }
        
        return parts;
    }
    
    struct ResolvedOptions {
        std::string locale;
        std::string type;
        std::string style;
    };
    
    ResolvedOptions resolvedOptions() const {
        ResolvedOptions opts;
        opts.locale = locale_;
        
        switch (options_.type) {
            case ListFormatType::Conjunction: opts.type = "conjunction"; break;
            case ListFormatType::Disjunction: opts.type = "disjunction"; break;
            case ListFormatType::Unit: opts.type = "unit"; break;
        }
        
        switch (options_.style) {
            case ListFormatStyle::Long: opts.style = "long"; break;
            case ListFormatStyle::Short: opts.style = "short"; break;
            case ListFormatStyle::Narrow: opts.style = "narrow"; break;
        }
        
        return opts;
    }
    
    static std::vector<std::string> supportedLocales() {
        return {"en", "en-US", "es", "fr", "de", "ja", "zh"};
    }

private:
    std::string getConnector() const {
        switch (options_.type) {
            case ListFormatType::Conjunction:
                switch (options_.style) {
                    case ListFormatStyle::Long: return " and ";
                    case ListFormatStyle::Short: return " & ";
                    case ListFormatStyle::Narrow: return ", ";
                }
                break;
            case ListFormatType::Disjunction:
                switch (options_.style) {
                    case ListFormatStyle::Long: return " or ";
                    case ListFormatStyle::Short: return " or ";
                    case ListFormatStyle::Narrow: return "/";
                }
                break;
            case ListFormatType::Unit:
                return ", ";
        }
        return " and ";
    }
    
    std::string getSeparator() const {
        return ", ";
    }
    
    std::string locale_;
    ListFormatOptions options_;
};

} // namespace Zepra::Runtime
