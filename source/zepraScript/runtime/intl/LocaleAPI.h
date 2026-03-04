/**
 * @file LocaleAPI.h
 * @brief Locale Implementation
 */

#pragma once

#include <string>
#include <vector>
#include <optional>
#include <map>

namespace Zepra::Runtime {

// =============================================================================
// Locale
// =============================================================================

class Locale {
public:
    Locale() : language_("en") {}
    
    explicit Locale(const std::string& tag) { parse(tag); }
    
    Locale(const std::string& language, const std::string& region)
        : language_(language), region_(region) {}
    
    // Properties
    const std::string& language() const { return language_; }
    const std::string& region() const { return region_; }
    const std::string& script() const { return script_; }
    const std::string& calendar() const { return calendar_; }
    const std::string& numberingSystem() const { return numberingSystem_; }
    
    std::string baseName() const {
        std::string result = language_;
        if (!script_.empty()) result += "-" + script_;
        if (!region_.empty()) result += "-" + region_;
        return result;
    }
    
    std::string toString() const {
        std::string result = baseName();
        if (!calendar_.empty()) result += "-u-ca-" + calendar_;
        if (!numberingSystem_.empty()) result += "-nu-" + numberingSystem_;
        return result;
    }
    
    // Modifiers
    Locale maximize() const {
        Locale result = *this;
        if (result.script_.empty()) {
            result.script_ = getDefaultScript(language_);
        }
        if (result.region_.empty()) {
            result.region_ = getDefaultRegion(language_);
        }
        return result;
    }
    
    Locale minimize() const {
        Locale result = *this;
        if (result.script_ == getDefaultScript(language_)) {
            result.script_.clear();
        }
        if (result.region_ == getDefaultRegion(language_)) {
            result.region_.clear();
        }
        return result;
    }
    
    // Static
    static std::vector<std::string> supportedLocales() {
        return {"en", "en-US", "en-GB", "es", "es-ES", "fr", "fr-FR", "de", "de-DE",
                "it", "it-IT", "ja", "ja-JP", "ko", "ko-KR", "zh", "zh-CN", "zh-TW",
                "pt", "pt-BR", "ru", "ru-RU", "ar", "ar-SA", "hi", "hi-IN"};
    }

private:
    void parse(const std::string& tag) {
        size_t pos = 0;
        size_t end = tag.find('-');
        language_ = tag.substr(0, end);
        
        if (end == std::string::npos) return;
        pos = end + 1;
        
        end = tag.find('-', pos);
        std::string part = tag.substr(pos, end - pos);
        
        if (part.length() == 4) {
            script_ = part;
            if (end != std::string::npos) {
                pos = end + 1;
                end = tag.find('-', pos);
                part = tag.substr(pos, end - pos);
            }
        }
        
        if (part.length() == 2) {
            region_ = part;
        }
    }
    
    static std::string getDefaultScript(const std::string& lang) {
        static const std::map<std::string, std::string> defaults = {
            {"en", "Latn"}, {"zh", "Hans"}, {"ja", "Jpan"}, {"ko", "Kore"},
            {"ar", "Arab"}, {"hi", "Deva"}, {"ru", "Cyrl"}
        };
        auto it = defaults.find(lang);
        return it != defaults.end() ? it->second : "Latn";
    }
    
    static std::string getDefaultRegion(const std::string& lang) {
        static const std::map<std::string, std::string> defaults = {
            {"en", "US"}, {"es", "ES"}, {"fr", "FR"}, {"de", "DE"},
            {"it", "IT"}, {"ja", "JP"}, {"ko", "KR"}, {"zh", "CN"},
            {"pt", "BR"}, {"ru", "RU"}, {"ar", "SA"}, {"hi", "IN"}
        };
        auto it = defaults.find(lang);
        return it != defaults.end() ? it->second : "";
    }
    
    std::string language_;
    std::string region_;
    std::string script_;
    std::string calendar_;
    std::string numberingSystem_;
};

// =============================================================================
// Locale Matcher
// =============================================================================

class LocaleMatcher {
public:
    enum class Algorithm { Lookup, BestFit };
    
    static std::string match(const std::vector<std::string>& requested,
                            const std::vector<std::string>& available,
                            const std::string& defaultLocale = "en",
                            Algorithm algo = Algorithm::BestFit) {
        for (const auto& req : requested) {
            for (const auto& avail : available) {
                if (matchesLocale(req, avail)) return avail;
            }
        }
        
        for (const auto& req : requested) {
            Locale reqLoc(req);
            for (const auto& avail : available) {
                Locale availLoc(avail);
                if (reqLoc.language() == availLoc.language()) return avail;
            }
        }
        
        return defaultLocale;
    }

private:
    static bool matchesLocale(const std::string& a, const std::string& b) {
        return a == b || a.find(b) == 0 || b.find(a) == 0;
    }
};

} // namespace Zepra::Runtime
