/**
 * @file RegExpAPI.h
 * @brief Regular Expression Engine Implementation
 * 
 * ECMAScript RegExp based on:
 * - JSC RegExp.h and Yarr
 * - SpiderMonkey RegExp.cpp
 * - ECMA-262 21.2 RegExp Objects
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <cstdint>
#include <unordered_map>
#include <regex>

namespace Zepra::Runtime {

// =============================================================================
// RegExp Flags
// =============================================================================

struct RegExpFlags {
    bool global = false;        // g - Global search
    bool ignoreCase = false;    // i - Case-insensitive
    bool multiline = false;     // m - Multi-line mode
    bool dotAll = false;        // s - DotAll mode
    bool unicode = false;       // u - Unicode mode
    bool unicodeSets = false;   // v - Unicode sets mode
    bool sticky = false;        // y - Sticky mode
    bool hasIndices = false;    // d - Has indices
    
    static RegExpFlags parse(const std::string& flagStr) {
        RegExpFlags flags;
        for (char c : flagStr) {
            switch (c) {
                case 'g': flags.global = true; break;
                case 'i': flags.ignoreCase = true; break;
                case 'm': flags.multiline = true; break;
                case 's': flags.dotAll = true; break;
                case 'u': flags.unicode = true; break;
                case 'v': flags.unicodeSets = true; break;
                case 'y': flags.sticky = true; break;
                case 'd': flags.hasIndices = true; break;
            }
        }
        return flags;
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
    
    bool eitherUnicode() const { return unicode || unicodeSets; }
};

// =============================================================================
// Match Result
// =============================================================================

struct RegExpCapture {
    std::string value;
    size_t start = 0;
    size_t end = 0;
    bool matched = false;
    
    size_t length() const { return end - start; }
};

struct RegExpMatch {
    std::string fullMatch;
    size_t index = 0;                              // Start index
    size_t endIndex = 0;                           // End index
    std::string input;                             // Original input
    std::vector<RegExpCapture> captures;           // Capture groups
    std::unordered_map<std::string, RegExpCapture> namedCaptures;
    
    bool empty() const { return fullMatch.empty() && !captures.empty(); }
    
    size_t captureCount() const { return captures.size(); }
    
    std::optional<RegExpCapture> getCapture(size_t index) const {
        if (index < captures.size()) {
            return captures[index];
        }
        return std::nullopt;
    }
    
    std::optional<RegExpCapture> getNamedCapture(const std::string& name) const {
        auto it = namedCaptures.find(name);
        if (it != namedCaptures.end()) {
            return it->second;
        }
        return std::nullopt;
    }
};

// =============================================================================
// RegExp Bytecode (for interpretation)
// =============================================================================

enum class RegExpOp : uint8_t {
    Match,              // Match complete
    Fail,               // Match failed
    Char,               // Match single character
    CharClass,          // Match character class
    CharClassNegated,   // Match negated character class
    Any,                // Match any character
    WordBoundary,       // \b
    NonWordBoundary,    // \B
    Start,              // ^
    End,                // $
    Jump,               // Unconditional jump
    Split,              // Non-deterministic split
    Save,               // Save position for capture
    BackReference,      // Back reference \1, \2, etc.
    NamedBackReference, // Named back reference \k<name>
    Lookahead,          // (?=...)
    NegativeLookahead,  // (?!...)
    Lookbehind,         // (?<=...)
    NegativeLookbehind  // (?<!...)
};

struct RegExpInstruction {
    RegExpOp op;
    int32_t arg1 = 0;
    int32_t arg2 = 0;
    std::string strArg;
};

// =============================================================================
// Compiled RegExp
// =============================================================================

class RegExp {
public:
    RegExp(std::string pattern, RegExpFlags flags = {})
        : pattern_(std::move(pattern))
        , flags_(flags) {
        compile();
    }
    
    RegExp(std::string pattern, const std::string& flagStr)
        : pattern_(std::move(pattern))
        , flags_(RegExpFlags::parse(flagStr)) {
        compile();
    }
    
    // Pattern and flags
    const std::string& pattern() const { return pattern_; }
    const RegExpFlags& flags() const { return flags_; }
    
    // Validity
    bool isValid() const { return !hasError_; }
    const std::string& errorMessage() const { return errorMessage_; }
    
    // Source string representation
    std::string source() const {
        std::string escaped;
        for (char c : pattern_) {
            if (c == '/') escaped += "\\/";
            else escaped += c;
        }
        return "/" + escaped + "/" + flags_.toString();
    }
    
    // Number of capture groups
    size_t numSubpatterns() const { return numCaptures_; }
    
    // Named capture groups
    bool hasNamedCaptures() const { return !namedCaptures_.empty(); }
    
    std::optional<size_t> getNamedCaptureIndex(const std::string& name) const {
        auto it = namedCaptures_.find(name);
        return it != namedCaptures_.end() ? std::optional<size_t>(it->second) : std::nullopt;
    }
    
    // Execution
    std::optional<RegExpMatch> exec(const std::string& input, size_t startIndex = 0) {
        if (!isValid()) return std::nullopt;
        
        size_t searchStart = flags_.sticky ? startIndex : 0;
        
        try {
            std::regex::flag_type regexFlags = std::regex::ECMAScript;
            if (flags_.ignoreCase) regexFlags |= std::regex::icase;
            if (flags_.multiline) regexFlags |= std::regex::multiline;
            
            std::regex re(pattern_, regexFlags);
            std::smatch match;
            
            std::string searchInput = input.substr(searchStart);
            
            if (std::regex_search(searchInput, match, re)) {
                RegExpMatch result;
                result.fullMatch = match[0].str();
                result.index = searchStart + match.position(0);
                result.endIndex = result.index + match[0].length();
                result.input = input;
                
                for (size_t i = 0; i < match.size(); ++i) {
                    RegExpCapture cap;
                    cap.matched = match[i].matched;
                    if (cap.matched) {
                        cap.value = match[i].str();
                        cap.start = searchStart + match.position(i);
                        cap.end = cap.start + match[i].length();
                    }
                    result.captures.push_back(cap);
                }
                
                for (const auto& [name, idx] : namedCaptures_) {
                    if (idx < result.captures.size()) {
                        result.namedCaptures[name] = result.captures[idx];
                    }
                }
                
                if (flags_.global || flags_.sticky) {
                    lastIndex_ = result.endIndex;
                }
                
                return result;
            }
        } catch (const std::regex_error&) {
            // Fallback failed
        }
        
        if (flags_.sticky) {
            lastIndex_ = 0;
        }
        
        return std::nullopt;
    }
    
    // Test if pattern matches
    bool test(const std::string& input, size_t startIndex = 0) {
        return exec(input, startIndex).has_value();
    }
    
    // Match all occurrences
    std::vector<RegExpMatch> matchAll(const std::string& input) {
        std::vector<RegExpMatch> results;
        size_t start = 0;
        
        while (start <= input.length()) {
            auto match = exec(input, start);
            if (!match) break;
            
            results.push_back(*match);
            
            if (match->fullMatch.empty()) {
                start = match->endIndex + 1;
            } else {
                start = match->endIndex;
            }
            
            if (!flags_.global) break;
        }
        
        return results;
    }
    
    // Replace
    std::string replace(const std::string& input, const std::string& replacement) {
        auto match = exec(input);
        if (!match) return input;
        
        std::string result = input.substr(0, match->index);
        result += processReplacement(replacement, *match);
        result += input.substr(match->endIndex);
        
        return result;
    }
    
    std::string replaceAll(const std::string& input, const std::string& replacement) {
        if (!flags_.global) {
            return replace(input, replacement);
        }
        
        std::string result;
        size_t lastEnd = 0;
        
        for (const auto& match : matchAll(input)) {
            result += input.substr(lastEnd, match.index - lastEnd);
            result += processReplacement(replacement, match);
            lastEnd = match.endIndex;
        }
        
        result += input.substr(lastEnd);
        return result;
    }
    
    // Split
    std::vector<std::string> split(const std::string& input, size_t limit = SIZE_MAX) {
        std::vector<std::string> results;
        size_t lastEnd = 0;
        
        for (const auto& match : matchAll(input)) {
            if (results.size() >= limit) break;
            
            results.push_back(input.substr(lastEnd, match.index - lastEnd));
            
            for (size_t i = 1; i < match.captures.size() && results.size() < limit; ++i) {
                if (match.captures[i].matched) {
                    results.push_back(match.captures[i].value);
                }
            }
            
            lastEnd = match.endIndex;
        }
        
        if (results.size() < limit) {
            results.push_back(input.substr(lastEnd));
        }
        
        return results;
    }
    
    // Search (returns index or -1)
    int search(const std::string& input) {
        auto match = exec(input);
        return match ? static_cast<int>(match->index) : -1;
    }
    
    // lastIndex property (for global/sticky)
    size_t lastIndex() const { return lastIndex_; }
    void setLastIndex(size_t idx) { lastIndex_ = idx; }
    
private:
    void compile() {
        hasError_ = false;
        numCaptures_ = 0;
        namedCaptures_.clear();
        
        try {
            std::regex::flag_type regexFlags = std::regex::ECMAScript;
            if (flags_.ignoreCase) regexFlags |= std::regex::icase;
            
            std::regex re(pattern_, regexFlags);
            numCaptures_ = re.mark_count();
            
            parseNamedCaptures();
        } catch (const std::regex_error& e) {
            hasError_ = true;
            errorMessage_ = e.what();
        }
    }
    
    void parseNamedCaptures() {
        size_t pos = 0;
        size_t captureIndex = 1;
        
        while ((pos = pattern_.find("(?<", pos)) != std::string::npos) {
            size_t nameStart = pos + 3;
            size_t nameEnd = pattern_.find('>', nameStart);
            
            if (nameEnd != std::string::npos) {
                std::string name = pattern_.substr(nameStart, nameEnd - nameStart);
                namedCaptures_[name] = captureIndex++;
            }
            
            pos = nameEnd + 1;
        }
    }
    
    std::string processReplacement(const std::string& replacement, const RegExpMatch& match) {
        std::string result;
        
        for (size_t i = 0; i < replacement.length(); ++i) {
            if (replacement[i] == '$' && i + 1 < replacement.length()) {
                char next = replacement[i + 1];
                
                if (next == '$') {
                    result += '$';
                    ++i;
                } else if (next == '&') {
                    result += match.fullMatch;
                    ++i;
                } else if (next == '`') {
                    result += match.input.substr(0, match.index);
                    ++i;
                } else if (next == '\'') {
                    result += match.input.substr(match.endIndex);
                    ++i;
                } else if (next >= '0' && next <= '9') {
                    size_t captureIdx = next - '0';
                    if (i + 2 < replacement.length() && replacement[i + 2] >= '0' && replacement[i + 2] <= '9') {
                        captureIdx = captureIdx * 10 + (replacement[i + 2] - '0');
                        ++i;
                    }
                    if (captureIdx < match.captures.size() && match.captures[captureIdx].matched) {
                        result += match.captures[captureIdx].value;
                    }
                    ++i;
                } else if (next == '<') {
                    size_t nameEnd = replacement.find('>', i + 2);
                    if (nameEnd != std::string::npos) {
                        std::string name = replacement.substr(i + 2, nameEnd - i - 2);
                        auto cap = match.getNamedCapture(name);
                        if (cap && cap->matched) {
                            result += cap->value;
                        }
                        i = nameEnd;
                    } else {
                        result += replacement[i];
                    }
                } else {
                    result += replacement[i];
                }
            } else {
                result += replacement[i];
            }
        }
        
        return result;
    }
    
    std::string pattern_;
    RegExpFlags flags_;
    bool hasError_ = false;
    std::string errorMessage_;
    size_t numCaptures_ = 0;
    std::unordered_map<std::string, size_t> namedCaptures_;
    size_t lastIndex_ = 0;
};

// =============================================================================
// RegExp Factory
// =============================================================================

inline std::shared_ptr<RegExp> createRegExp(const std::string& pattern, const std::string& flags = "") {
    return std::make_shared<RegExp>(pattern, flags);
}

} // namespace Zepra::Runtime
