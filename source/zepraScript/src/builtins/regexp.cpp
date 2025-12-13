/**
 * @file regexp.cpp
 * @brief JavaScript RegExp builtin implementation
 */

#include "zeprascript/builtins/regexp.hpp"
#include "zeprascript/runtime/function.hpp"

namespace Zepra::Builtins {

// =============================================================================
// RegExpObject Implementation
// =============================================================================

RegExpObject::RegExpObject(const std::string& pattern, const std::string& flags)
    : Object(ObjectType::RegExp)
    , pattern_(pattern)
    , flags_(flags) {
    
    // Parse flags
    for (char f : flags) {
        switch (f) {
            case 'g': global_ = true; break;
            case 'i': ignoreCase_ = true; break;
            case 'm': multiline_ = true; break;
        }
    }
    
    // Compile regex
    try {
        regex_ = std::regex(pattern, parseFlags());
    } catch (const std::regex_error&) {
        // Invalid pattern - leave regex empty
    }
}

std::regex_constants::syntax_option_type RegExpObject::parseFlags() const {
    auto opts = std::regex_constants::ECMAScript;
    if (ignoreCase_) opts |= std::regex_constants::icase;
    if (multiline_) opts |= std::regex_constants::multiline;
    return opts;
}

bool RegExpObject::test(const std::string& str) const {
    return std::regex_search(str, regex_);
}

Value RegExpObject::exec(const std::string& str) const {
    std::smatch match;
    if (std::regex_search(str, match, regex_)) {
        // Return array with match info
        Runtime::Array* result = new Runtime::Array();
        for (size_t i = 0; i < match.size(); ++i) {
            result->push(Value::string(new Runtime::String(match[i].str())));
        }
        return Value::object(result);
    }
    return Value::null();
}

std::string RegExpObject::replace(const std::string& str, const std::string& replacement) const {
    if (global_) {
        return std::regex_replace(str, regex_, replacement);
    } else {
        return std::regex_replace(str, regex_, replacement, 
            std::regex_constants::format_first_only);
    }
}

std::vector<std::string> RegExpObject::match(const std::string& str) const {
    std::vector<std::string> result;
    
    if (global_) {
        auto begin = std::sregex_iterator(str.begin(), str.end(), regex_);
        auto end = std::sregex_iterator();
        for (auto it = begin; it != end; ++it) {
            result.push_back((*it)[0].str());
        }
    } else {
        std::smatch m;
        if (std::regex_search(str, m, regex_)) {
            result.push_back(m[0].str());
        }
    }
    
    return result;
}

int RegExpObject::search(const std::string& str) const {
    std::smatch match;
    if (std::regex_search(str, match, regex_)) {
        return static_cast<int>(match.position(0));
    }
    return -1;
}

std::vector<std::string> RegExpObject::split(const std::string& str, int limit) const {
    std::vector<std::string> result;
    std::sregex_token_iterator it(str.begin(), str.end(), regex_, -1);
    std::sregex_token_iterator end;
    
    int count = 0;
    while (it != end && (limit < 0 || count < limit)) {
        result.push_back(*it++);
        count++;
    }
    
    return result;
}

// =============================================================================
// RegExpBuiltin Implementation
// =============================================================================

Value RegExpBuiltin::constructor(Runtime::Context*, const std::vector<Value>& args) {
    std::string pattern = "";
    std::string flags = "";
    
    if (!args.empty() && args[0].isString()) {
        pattern = static_cast<Runtime::String*>(args[0].asObject())->value();
    }
    if (args.size() > 1 && args[1].isString()) {
        flags = static_cast<Runtime::String*>(args[1].asObject())->value();
    }
    
    return Value::object(new RegExpObject(pattern, flags));
}

Value RegExpBuiltin::test(Runtime::Context*, const std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isObject()) return Value::boolean(false);
    
    RegExpObject* re = dynamic_cast<RegExpObject*>(args[0].asObject());
    if (!re) return Value::boolean(false);
    
    std::string str = "";
    if (args[1].isString()) {
        str = static_cast<Runtime::String*>(args[1].asObject())->value();
    }
    
    return Value::boolean(re->test(str));
}

Value RegExpBuiltin::exec(Runtime::Context*, const std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isObject()) return Value::null();
    
    RegExpObject* re = dynamic_cast<RegExpObject*>(args[0].asObject());
    if (!re) return Value::null();
    
    std::string str = "";
    if (args[1].isString()) {
        str = static_cast<Runtime::String*>(args[1].asObject())->value();
    }
    
    return re->exec(str);
}

Object* RegExpBuiltin::createRegExpPrototype() {
    Object* proto = new Object();
    return proto;
}

} // namespace Zepra::Builtins
