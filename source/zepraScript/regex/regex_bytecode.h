/**
 * @file regex_bytecode.h
 * @brief Regex bytecode instruction set
 *
 * Defines the instruction set for the regex VM.
 * The regex compiler converts patterns into this bytecode,
 * which the regex engine then executes via an NFA/backtracking VM.
 *
 * Ref: SpiderMonkey irregexp, RE2 NFA
 */

#pragma once

#include <cstdint>
#include <vector>
#include <string>

namespace Zepra::Regex {

// =============================================================================
// Opcodes
// =============================================================================

enum class RegexOp : uint8_t {
    // Matching
    Char,           // Match literal character (operand: char32_t)
    CharIgnoreCase, // Match character case-insensitive
    AnyChar,        // Match any character (except newline unless dotAll)
    CharClass,      // Match character class (operand: class index)
    NegCharClass,   // Match negated character class

    // Quantifiers
    Split,          // Fork: try both paths (operand: offset_a, offset_b)
    Jump,           // Unconditional jump (operand: offset)
    // Greedy loop: Split with primary = body, secondary = exit
    // Lazy loop: Split with primary = exit, secondary = body

    // Assertions
    LineStart,      // ^ or \A
    LineEnd,        // $ or \Z
    WordBoundary,   // \b
    NotWordBoundary,// \B

    // Backreferences
    SaveStart,      // Save capture group start (operand: group index)
    SaveEnd,        // Save capture group end
    BackRef,        // Match backreference (operand: group index)

    // Lookahead/behind
    LookaheadStart, // (?= or (?!   (operand: is_negative)
    LookaheadEnd,   // End of lookahead (success)
    LookbehindStart,// (?<= or (?<! (operand: is_negative, width)
    LookbehindEnd,

    // Count
    RepeatStart,    // Start counted repetition (operand: min, max)
    RepeatEnd,      // End counted repetition (operand: loop id)

    // Control
    Match,          // Successful match
    Fail,           // Force failure (backtrack)
    Progress,       // Empty-string loop guard (prevents infinite empty loops)
};

// =============================================================================
// Instruction
// =============================================================================

struct RegexInstr {
    RegexOp op;
    union {
        char32_t ch;        // For Char
        int32_t offset;     // For Jump/Split
        uint16_t groupIdx;  // For Save/BackRef
        uint16_t classIdx;  // For CharClass
        struct { uint16_t min; uint16_t max; } repeat;
        struct { int32_t offsetA; int32_t offsetB; } split;
    };

    RegexInstr() : op(RegexOp::Fail), ch(0) {}
    explicit RegexInstr(RegexOp opc) : op(opc), ch(0) {}
};

// =============================================================================
// Character class
// =============================================================================

struct CharRange {
    char32_t lo;
    char32_t hi; // inclusive

    bool contains(char32_t c) const { return c >= lo && c <= hi; }
};

struct CharClass {
    std::vector<CharRange> ranges;
    bool negated = false;

    bool contains(char32_t c) const {
        bool inRange = false;
        for (const auto& r : ranges) {
            if (r.contains(c)) { inRange = true; break; }
        }
        return negated ? !inRange : inRange;
    }
};

// =============================================================================
// Compiled regex program
// =============================================================================

struct RegexProgram {
    std::vector<RegexInstr> code;
    std::vector<CharClass> classes;
    uint32_t numGroups = 0;    // Number of capture groups (group 0 = whole match)
    std::vector<std::string> groupNames; // Named capture group names

    size_t size() const { return code.size(); }
    const RegexInstr& at(size_t i) const { return code[i]; }
};

// =============================================================================
// Flags
// =============================================================================

struct RegexFlags {
    bool global = false;        // g
    bool ignoreCase = false;    // i
    bool multiline = false;     // m
    bool dotAll = false;        // s
    bool unicode = false;       // u
    bool sticky = false;        // y
    bool hasIndices = false;    // d
};

} // namespace Zepra::Regex
