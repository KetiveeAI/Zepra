/**
 * @file RegExpExtensionsAPI.h
 * @brief RegExp Extensions Implementation
 */

#pragma once

#include <string>
#include <vector>
#include <set>
#include <map>
#include <regex>
#include <optional>

namespace Zepra::Runtime {

// =============================================================================
// Unicode Property
// =============================================================================

class UnicodeProperty {
public:
    static bool isProperty(char32_t codepoint, const std::string& property) {
        if (property == "Letter" || property == "L") return isLetter(codepoint);
        if (property == "Number" || property == "N") return isNumber(codepoint);
        if (property == "Punctuation" || property == "P") return isPunctuation(codepoint);
        if (property == "Symbol" || property == "S") return isSymbol(codepoint);
        if (property == "Separator" || property == "Z") return isSeparator(codepoint);
        if (property == "ASCII") return codepoint <= 0x7F;
        if (property == "Emoji") return isEmoji(codepoint);
        return false;
    }
    
    static bool hasScript(char32_t codepoint, const std::string& script) {
        if (script == "Latin") return isLatin(codepoint);
        if (script == "Greek") return isGreek(codepoint);
        if (script == "Cyrillic") return isCyrillic(codepoint);
        if (script == "Han") return isHan(codepoint);
        if (script == "Hiragana") return isHiragana(codepoint);
        if (script == "Katakana") return isKatakana(codepoint);
        if (script == "Arabic") return isArabic(codepoint);
        if (script == "Devanagari") return isDevanagari(codepoint);
        return false;
    }

private:
    static bool isLetter(char32_t c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c > 0x7F; }
    static bool isNumber(char32_t c) { return c >= '0' && c <= '9'; }
    static bool isPunctuation(char32_t c) { return (c >= 0x21 && c <= 0x2F) || (c >= 0x3A && c <= 0x40); }
    static bool isSymbol(char32_t c) { return (c >= 0x24 && c <= 0x2B) || c == 0x5E || c == 0x60; }
    static bool isSeparator(char32_t c) { return c == 0x20 || c == 0xA0 || c == 0x2028 || c == 0x2029; }
    static bool isEmoji(char32_t c) { return (c >= 0x1F600 && c <= 0x1F64F) || (c >= 0x1F300 && c <= 0x1F5FF); }
    static bool isLatin(char32_t c) { return (c >= 0x0041 && c <= 0x024F); }
    static bool isGreek(char32_t c) { return (c >= 0x0370 && c <= 0x03FF); }
    static bool isCyrillic(char32_t c) { return (c >= 0x0400 && c <= 0x04FF); }
    static bool isHan(char32_t c) { return (c >= 0x4E00 && c <= 0x9FFF); }
    static bool isHiragana(char32_t c) { return (c >= 0x3040 && c <= 0x309F); }
    static bool isKatakana(char32_t c) { return (c >= 0x30A0 && c <= 0x30FF); }
    static bool isArabic(char32_t c) { return (c >= 0x0600 && c <= 0x06FF); }
    static bool isDevanagari(char32_t c) { return (c >= 0x0900 && c <= 0x097F); }
};

// =============================================================================
// Character Set (for /v flag)
// =============================================================================

class CharacterSet {
public:
    void add(char32_t c) { chars_.insert(c); }
    void addRange(char32_t start, char32_t end) {
        for (char32_t c = start; c <= end; ++c) chars_.insert(c);
    }
    void remove(char32_t c) { chars_.erase(c); }
    bool contains(char32_t c) const { return chars_.find(c) != chars_.end(); }
    
    CharacterSet unionWith(const CharacterSet& other) const {
        CharacterSet result = *this;
        for (char32_t c : other.chars_) result.add(c);
        return result;
    }
    
    CharacterSet intersect(const CharacterSet& other) const {
        CharacterSet result;
        for (char32_t c : chars_) {
            if (other.contains(c)) result.add(c);
        }
        return result;
    }
    
    CharacterSet subtract(const CharacterSet& other) const {
        CharacterSet result = *this;
        for (char32_t c : other.chars_) result.remove(c);
        return result;
    }
    
    size_t size() const { return chars_.size(); }

private:
    std::set<char32_t> chars_;
};

// =============================================================================
// Extended RegExp Flags
// =============================================================================

struct RegExpFlags {
    bool global = false;       // g
    bool ignoreCase = false;   // i
    bool multiline = false;    // m
    bool dotAll = false;       // s
    bool unicode = false;      // u
    bool unicodeSets = false;  // v
    bool sticky = false;       // y
    bool hasIndices = false;   // d
    
    static RegExpFlags parse(const std::string& flags) {
        RegExpFlags result;
        for (char c : flags) {
            switch (c) {
                case 'g': result.global = true; break;
                case 'i': result.ignoreCase = true; break;
                case 'm': result.multiline = true; break;
                case 's': result.dotAll = true; break;
                case 'u': result.unicode = true; break;
                case 'v': result.unicodeSets = true; break;
                case 'y': result.sticky = true; break;
                case 'd': result.hasIndices = true; break;
            }
        }
        return result;
    }
    
    std::string toString() const {
        std::string result;
        if (hasIndices) result += 'd';
        if (global) result += 'g';
        if (ignoreCase) result += 'i';
        if (multiline) result += 'm';
        if (dotAll) result += 's';
        if (unicode) result += 'u';
        if (unicodeSets) result += 'v';
        if (sticky) result += 'y';
        return result;
    }
};

// =============================================================================
// Extended Match Result
// =============================================================================

struct ExtendedMatchResult {
    std::string match;
    size_t index;
    std::string input;
    std::map<std::string, std::string> groups;
    
    struct Indices {
        std::pair<size_t, size_t> range;
        std::map<std::string, std::pair<size_t, size_t>> groups;
    };
    std::optional<Indices> indices;
};

// =============================================================================
// Unicode Property Escape
// =============================================================================

class UnicodePropertyEscape {
public:
    static CharacterSet expand(const std::string& property) {
        CharacterSet result;
        
        if (property == "ASCII_Hex_Digit") {
            result.addRange('0', '9');
            result.addRange('A', 'F');
            result.addRange('a', 'f');
        } else if (property == "White_Space") {
            result.add(' ');
            result.add('\t');
            result.add('\n');
            result.add('\r');
            result.add('\f');
            result.add('\v');
        }
        
        return result;
    }
};

} // namespace Zepra::Runtime
