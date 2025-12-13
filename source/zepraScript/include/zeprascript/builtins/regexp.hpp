#pragma once

/**
 * @file regexp.hpp
 * @brief JavaScript RegExp builtin
 */

#include "../config.hpp"
#include "../runtime/object.hpp"
#include "../runtime/value.hpp"
#include <regex>
#include <string>
#include <vector>

namespace Zepra::Runtime { class Context; }

namespace Zepra::Builtins {

using Runtime::Value;
using Runtime::Object;
using Runtime::ObjectType;

/**
 * @brief JavaScript RegExp object
 */
class RegExpObject : public Object {
public:
    RegExpObject(const std::string& pattern, const std::string& flags = "");
    
    // RegExp API
    bool test(const std::string& str) const;
    Value exec(const std::string& str) const;
    
    // Properties
    const std::string& pattern() const { return pattern_; }
    const std::string& flags() const { return flags_; }
    bool global() const { return global_; }
    bool ignoreCase() const { return ignoreCase_; }
    bool multiline() const { return multiline_; }
    int lastIndex() const { return lastIndex_; }
    void setLastIndex(int idx) { lastIndex_ = idx; }
    
    // String methods that use regex
    std::string replace(const std::string& str, const std::string& replacement) const;
    std::vector<std::string> match(const std::string& str) const;
    int search(const std::string& str) const;
    std::vector<std::string> split(const std::string& str, int limit = -1) const;
    
private:
    std::string pattern_;
    std::string flags_;
    std::regex regex_;
    bool global_ = false;
    bool ignoreCase_ = false;
    bool multiline_ = false;
    int lastIndex_ = 0;
    
    std::regex_constants::syntax_option_type parseFlags() const;
};

/**
 * @brief RegExp builtin functions
 */
class RegExpBuiltin {
public:
    static Value constructor(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value test(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value exec(Runtime::Context* ctx, const std::vector<Value>& args);
    
    static Object* createRegExpPrototype();
};

} // namespace Zepra::Builtins
