#pragma once

#include <string>
#include <regex>
#include <vector>
#include <optional>
#include <unordered_map>

namespace Zepra::Runtime {

class RegExpEnhancements {
public:
    static std::string escape(const std::string& str) {
        static const std::string metacharacters = R"([\^$.|?*+(){}])";
        std::string result;
        result.reserve(str.size() * 2);
        
        for (char c : str) {
            if (metacharacters.find(c) != std::string::npos) {
                result += '\\';
            }
            result += c;
        }
        return result;
    }

    static bool hasValidUnicodeFlag(const std::string& flags) {
        return flags.find('u') != std::string::npos || 
               flags.find('v') != std::string::npos;
    }

    static bool isValidFlags(const std::string& flags) {
        static const std::string validFlags = "dgimsuvy";
        for (char c : flags) {
            if (validFlags.find(c) == std::string::npos) return false;
        }
        if (flags.find('u') != std::string::npos && 
            flags.find('v') != std::string::npos) {
            return false;
        }
        return true;
    }
};

struct RegExpMatch {
    std::string match;
    size_t index;
    std::string input;
    std::vector<std::string> groups;
    std::unordered_map<std::string, std::string> namedGroups;
    std::optional<std::vector<std::pair<size_t, size_t>>> indices;
};

class EnhancedRegExp {
private:
    std::regex regex_;
    std::string source_;
    std::string flags_;
    bool hasIndices_;
    bool global_;
    bool ignoreCase_;
    bool multiline_;
    bool dotAll_;
    bool unicode_;
    bool unicodeSets_;
    bool sticky_;
    size_t lastIndex_ = 0;

public:
    EnhancedRegExp(const std::string& pattern, const std::string& flags = "")
        : source_(pattern), flags_(flags) {
        
        parseFlags();
        
        std::regex_constants::syntax_option_type opts = std::regex_constants::ECMAScript;
        if (ignoreCase_) opts |= std::regex_constants::icase;
        
        regex_ = std::regex(pattern, opts);
    }

    void parseFlags() {
        hasIndices_ = flags_.find('d') != std::string::npos;
        global_ = flags_.find('g') != std::string::npos;
        ignoreCase_ = flags_.find('i') != std::string::npos;
        multiline_ = flags_.find('m') != std::string::npos;
        dotAll_ = flags_.find('s') != std::string::npos;
        unicode_ = flags_.find('u') != std::string::npos;
        unicodeSets_ = flags_.find('v') != std::string::npos;
        sticky_ = flags_.find('y') != std::string::npos;
    }

    std::optional<RegExpMatch> exec(const std::string& str) {
        std::smatch match;
        std::string::const_iterator start = str.begin() + lastIndex_;
        
        if (!std::regex_search(start, str.end(), match, regex_)) {
            if (global_ || sticky_) lastIndex_ = 0;
            return std::nullopt;
        }

        RegExpMatch result;
        result.match = match[0].str();
        result.index = match.position() + lastIndex_;
        result.input = str;

        for (size_t i = 1; i < match.size(); ++i) {
            result.groups.push_back(match[i].str());
        }

        if (hasIndices_) {
            result.indices = std::vector<std::pair<size_t, size_t>>();
            for (size_t i = 0; i < match.size(); ++i) {
                size_t start = match.position(i) + lastIndex_;
                size_t end = start + match[i].length();
                result.indices->push_back({start, end});
            }
        }

        if (global_ || sticky_) {
            lastIndex_ = result.index + result.match.length();
        }

        return result;
    }

    bool test(const std::string& str) {
        return exec(str).has_value();
    }

    std::vector<RegExpMatch> matchAll(const std::string& str) {
        std::vector<RegExpMatch> matches;
        size_t savedLastIndex = lastIndex_;
        lastIndex_ = 0;

        while (auto match = exec(str)) {
            matches.push_back(*match);
            if (match->match.empty()) lastIndex_++;
        }

        lastIndex_ = savedLastIndex;
        return matches;
    }

    std::string replace(const std::string& str, const std::string& replacement) {
        if (global_) {
            return std::regex_replace(str, regex_, replacement);
        }
        return std::regex_replace(str, regex_, replacement, 
                                   std::regex_constants::format_first_only);
    }

    std::string replaceAll(const std::string& str, const std::string& replacement) {
        return std::regex_replace(str, regex_, replacement);
    }

    std::vector<std::string> split(const std::string& str, size_t limit = SIZE_MAX) {
        std::vector<std::string> result;
        std::sregex_token_iterator it(str.begin(), str.end(), regex_, -1);
        std::sregex_token_iterator end;
        
        for (; it != end && result.size() < limit; ++it) {
            result.push_back(*it);
        }
        return result;
    }

    const std::string& source() const { return source_; }
    const std::string& flags() const { return flags_; }
    bool hasIndices() const { return hasIndices_; }
    bool global() const { return global_; }
    bool ignoreCase() const { return ignoreCase_; }
    bool multiline() const { return multiline_; }
    bool dotAll() const { return dotAll_; }
    bool unicode() const { return unicode_; }
    bool unicodeSets() const { return unicodeSets_; }
    bool sticky() const { return sticky_; }
    size_t lastIndex() const { return lastIndex_; }
    void setLastIndex(size_t idx) { lastIndex_ = idx; }
};

class UnicodeSetNotation {
public:
    static std::string parsePropertyEscape(const std::string& property) {
        static const std::unordered_map<std::string, std::string> properties = {
            {"ASCII", R"([\x00-\x7F])"},
            {"Alphabetic", R"([A-Za-z\xC0-\xFF])"},
            {"Decimal_Number", R"([0-9])"},
            {"White_Space", R"([\s])"},
            {"Emoji", R"([\x{1F600}-\x{1F64F}])"},
        };
        
        auto it = properties.find(property);
        return it != properties.end() ? it->second : "";
    }

    static std::string expandSetNotation(const std::string& pattern) {
        std::string result = pattern;

        size_t pos = 0;
        while ((pos = result.find("[[", pos)) != std::string::npos) {
            size_t end = result.find("]]", pos);
            if (end == std::string::npos) break;
            
            std::string inner = result.substr(pos + 2, end - pos - 2);
            result = result.replace(pos, end - pos + 2, "[" + inner + "]");
            pos++;
        }

        return result;
    }
};

}
