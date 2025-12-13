/**
 * @file string.cpp
 * @brief String builtin implementation
 */

#include "zeprascript/builtins/string.hpp"
#include "zeprascript/runtime/object.hpp"
#include "zeprascript/runtime/function.hpp"
#include "zeprascript/runtime/value.hpp"
#include <algorithm>
#include <sstream>
#include <cctype>
#include <regex>

namespace Zepra::Builtins {

// String.fromCharCode(...codes)
Runtime::Value StringBuiltin::fromCharCode(const Runtime::FunctionCallInfo& info) {
    std::string result;
    
    for (size_t i = 0; i < info.argumentCount(); i++) {
        if (info.argument(i).isNumber()) {
            int code = static_cast<int>(info.argument(i).asNumber());
            if (code >= 0 && code <= 0xFFFF) {
                result += static_cast<char>(code);
            }
        }
    }
    
    return Runtime::Value::string(new Runtime::String(result));
}

// String.prototype.charAt(index)
Runtime::Value StringBuiltin::charAt(const Runtime::FunctionCallInfo& info) {
    Runtime::Value thisVal = info.thisValue();
    std::string str = thisVal.toString();
    
    size_t index = 0;
    if (info.argumentCount() > 0 && info.argument(0).isNumber()) {
        index = static_cast<size_t>(info.argument(0).asNumber());
    }
    
    if (index >= str.length()) {
        return Runtime::Value::string(new Runtime::String(""));
    }
    
    return Runtime::Value::string(new Runtime::String(std::string(1, str[index])));
}

// String.prototype.charCodeAt(index)
Runtime::Value StringBuiltin::charCodeAt(const Runtime::FunctionCallInfo& info) {
    Runtime::Value thisVal = info.thisValue();
    std::string str = thisVal.toString();
    
    size_t index = 0;
    if (info.argumentCount() > 0 && info.argument(0).isNumber()) {
        index = static_cast<size_t>(info.argument(0).asNumber());
    }
    
    if (index >= str.length()) {
        return Runtime::Value::number(std::nan(""));
    }
    
    return Runtime::Value::number(static_cast<double>(static_cast<unsigned char>(str[index])));
}

// String.prototype.concat(...strings)
Runtime::Value StringBuiltin::concat(const Runtime::FunctionCallInfo& info) {
    Runtime::Value thisVal = info.thisValue();
    std::string result = thisVal.toString();
    
    for (size_t i = 0; i < info.argumentCount(); i++) {
        result += info.argument(i).toString();
    }
    
    return Runtime::Value::string(new Runtime::String(result));
}

// String.prototype.includes(searchString, position?)
Runtime::Value StringBuiltin::includes(const Runtime::FunctionCallInfo& info) {
    Runtime::Value thisVal = info.thisValue();
    std::string str = thisVal.toString();
    
    if (info.argumentCount() < 1) {
        return Runtime::Value::boolean(false);
    }
    
    std::string searchStr = info.argument(0).toString();
    size_t position = 0;
    
    if (info.argumentCount() > 1 && info.argument(1).isNumber()) {
        position = static_cast<size_t>(std::max(0.0, info.argument(1).asNumber()));
    }
    
    return Runtime::Value::boolean(str.find(searchStr, position) != std::string::npos);
}

// String.prototype.indexOf(searchString, position?)
Runtime::Value StringBuiltin::indexOf(const Runtime::FunctionCallInfo& info) {
    Runtime::Value thisVal = info.thisValue();
    std::string str = thisVal.toString();
    
    if (info.argumentCount() < 1) {
        return Runtime::Value::number(-1);
    }
    
    std::string searchStr = info.argument(0).toString();
    size_t position = 0;
    
    if (info.argumentCount() > 1 && info.argument(1).isNumber()) {
        position = static_cast<size_t>(std::max(0.0, info.argument(1).asNumber()));
    }
    
    size_t pos = str.find(searchStr, position);
    return Runtime::Value::number(pos == std::string::npos ? -1 : static_cast<double>(pos));
}

// String.prototype.lastIndexOf(searchString, position?)
Runtime::Value StringBuiltin::lastIndexOf(const Runtime::FunctionCallInfo& info) {
    Runtime::Value thisVal = info.thisValue();
    std::string str = thisVal.toString();
    
    if (info.argumentCount() < 1) {
        return Runtime::Value::number(-1);
    }
    
    std::string searchStr = info.argument(0).toString();
    size_t position = std::string::npos;
    
    if (info.argumentCount() > 1 && info.argument(1).isNumber()) {
        position = static_cast<size_t>(std::max(0.0, info.argument(1).asNumber()));
    }
    
    size_t pos = str.rfind(searchStr, position);
    return Runtime::Value::number(pos == std::string::npos ? -1 : static_cast<double>(pos));
}

// String.prototype.slice(start, end?)
Runtime::Value StringBuiltin::slice(const Runtime::FunctionCallInfo& info) {
    Runtime::Value thisVal = info.thisValue();
    std::string str = thisVal.toString();
    int64_t len = static_cast<int64_t>(str.length());
    
    int64_t start = 0;
    int64_t end = len;
    
    if (info.argumentCount() > 0 && info.argument(0).isNumber()) {
        start = static_cast<int64_t>(info.argument(0).asNumber());
        if (start < 0) start = std::max(len + start, int64_t(0));
    }
    
    if (info.argumentCount() > 1 && info.argument(1).isNumber()) {
        end = static_cast<int64_t>(info.argument(1).asNumber());
        if (end < 0) end = std::max(len + end, int64_t(0));
    }
    
    if (start >= end || start >= len) {
        return Runtime::Value::string(new Runtime::String(""));
    }
    
    return Runtime::Value::string(new Runtime::String(
        str.substr(static_cast<size_t>(start), static_cast<size_t>(end - start))
    ));
}

// String.prototype.substring(start, end?)
Runtime::Value StringBuiltin::substring(const Runtime::FunctionCallInfo& info) {
    Runtime::Value thisVal = info.thisValue();
    std::string str = thisVal.toString();
    size_t len = str.length();
    
    size_t start = 0;
    size_t end = len;
    
    if (info.argumentCount() > 0 && info.argument(0).isNumber()) {
        start = static_cast<size_t>(std::max(0.0, info.argument(0).asNumber()));
        start = std::min(start, len);
    }
    
    if (info.argumentCount() > 1 && info.argument(1).isNumber()) {
        end = static_cast<size_t>(std::max(0.0, info.argument(1).asNumber()));
        end = std::min(end, len);
    }
    
    if (start > end) std::swap(start, end);
    
    return Runtime::Value::string(new Runtime::String(str.substr(start, end - start)));
}

// String.prototype.toLowerCase()
Runtime::Value StringBuiltin::toLowerCase(const Runtime::FunctionCallInfo& info) {
    Runtime::Value thisVal = info.thisValue();
    std::string str = thisVal.toString();
    
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    
    return Runtime::Value::string(new Runtime::String(str));
}

// String.prototype.toUpperCase()
Runtime::Value StringBuiltin::toUpperCase(const Runtime::FunctionCallInfo& info) {
    Runtime::Value thisVal = info.thisValue();
    std::string str = thisVal.toString();
    
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);
    
    return Runtime::Value::string(new Runtime::String(str));
}

// String.prototype.trim()
Runtime::Value StringBuiltin::trim(const Runtime::FunctionCallInfo& info) {
    Runtime::Value thisVal = info.thisValue();
    std::string str = thisVal.toString();
    
    size_t start = str.find_first_not_of(" \t\n\r\f\v");
    if (start == std::string::npos) {
        return Runtime::Value::string(new Runtime::String(""));
    }
    
    size_t end = str.find_last_not_of(" \t\n\r\f\v");
    
    return Runtime::Value::string(new Runtime::String(str.substr(start, end - start + 1)));
}

// String.prototype.trimStart() / trimLeft()
Runtime::Value StringBuiltin::trimStart(const Runtime::FunctionCallInfo& info) {
    Runtime::Value thisVal = info.thisValue();
    std::string str = thisVal.toString();
    
    size_t start = str.find_first_not_of(" \t\n\r\f\v");
    if (start == std::string::npos) {
        return Runtime::Value::string(new Runtime::String(""));
    }
    
    return Runtime::Value::string(new Runtime::String(str.substr(start)));
}

// String.prototype.trimEnd() / trimRight()
Runtime::Value StringBuiltin::trimEnd(const Runtime::FunctionCallInfo& info) {
    Runtime::Value thisVal = info.thisValue();
    std::string str = thisVal.toString();
    
    size_t end = str.find_last_not_of(" \t\n\r\f\v");
    if (end == std::string::npos) {
        return Runtime::Value::string(new Runtime::String(""));
    }
    
    return Runtime::Value::string(new Runtime::String(str.substr(0, end + 1)));
}

// String.prototype.split(separator?, limit?)
Runtime::Value StringBuiltin::split(const Runtime::FunctionCallInfo& info) {
    Runtime::Value thisVal = info.thisValue();
    std::string str = thisVal.toString();
    std::vector<Runtime::Value> parts;
    
    if (info.argumentCount() == 0) {
        parts.push_back(Runtime::Value::string(new Runtime::String(str)));
        return Runtime::Value::object(new Runtime::Array(std::move(parts)));
    }
    
    std::string separator = info.argument(0).toString();
    size_t limit = SIZE_MAX;
    
    if (info.argumentCount() > 1 && info.argument(1).isNumber()) {
        limit = static_cast<size_t>(info.argument(1).asNumber());
    }
    
    if (separator.empty()) {
        for (size_t i = 0; i < str.length() && parts.size() < limit; i++) {
            parts.push_back(Runtime::Value::string(new Runtime::String(std::string(1, str[i]))));
        }
    } else {
        size_t pos = 0;
        size_t found;
        while ((found = str.find(separator, pos)) != std::string::npos && parts.size() < limit) {
            parts.push_back(Runtime::Value::string(new Runtime::String(str.substr(pos, found - pos))));
            pos = found + separator.length();
        }
        if (parts.size() < limit) {
            parts.push_back(Runtime::Value::string(new Runtime::String(str.substr(pos))));
        }
    }
    
    return Runtime::Value::object(new Runtime::Array(std::move(parts)));
}

// String.prototype.repeat(count)
Runtime::Value StringBuiltin::repeat(const Runtime::FunctionCallInfo& info) {
    Runtime::Value thisVal = info.thisValue();
    std::string str = thisVal.toString();
    
    if (info.argumentCount() < 1) {
        return Runtime::Value::string(new Runtime::String(""));
    }
    
    int count = static_cast<int>(info.argument(0).asNumber());
    if (count < 0) {
        return Runtime::Value::string(new Runtime::String(""));
    }
    
    std::string result;
    result.reserve(str.length() * count);
    for (int i = 0; i < count; i++) {
        result += str;
    }
    
    return Runtime::Value::string(new Runtime::String(result));
}

// String.prototype.startsWith(searchString, position?)
Runtime::Value StringBuiltin::startsWith(const Runtime::FunctionCallInfo& info) {
    Runtime::Value thisVal = info.thisValue();
    std::string str = thisVal.toString();
    
    if (info.argumentCount() < 1) {
        return Runtime::Value::boolean(false);
    }
    
    std::string searchStr = info.argument(0).toString();
    size_t position = 0;
    
    if (info.argumentCount() > 1 && info.argument(1).isNumber()) {
        position = static_cast<size_t>(std::max(0.0, info.argument(1).asNumber()));
    }
    
    return Runtime::Value::boolean(str.compare(position, searchStr.length(), searchStr) == 0);
}

// String.prototype.endsWith(searchString, length?)
Runtime::Value StringBuiltin::endsWith(const Runtime::FunctionCallInfo& info) {
    Runtime::Value thisVal = info.thisValue();
    std::string str = thisVal.toString();
    size_t len = str.length();
    
    if (info.argumentCount() < 1) {
        return Runtime::Value::boolean(false);
    }
    
    std::string searchStr = info.argument(0).toString();
    
    if (info.argumentCount() > 1 && info.argument(1).isNumber()) {
        len = static_cast<size_t>(std::max(0.0, info.argument(1).asNumber()));
        len = std::min(len, str.length());
    }
    
    if (searchStr.length() > len) {
        return Runtime::Value::boolean(false);
    }
    
    return Runtime::Value::boolean(
        str.compare(len - searchStr.length(), searchStr.length(), searchStr) == 0
    );
}

// String.prototype.padStart(targetLength, padString?)
Runtime::Value StringBuiltin::padStart(const Runtime::FunctionCallInfo& info) {
    Runtime::Value thisVal = info.thisValue();
    std::string str = thisVal.toString();
    
    if (info.argumentCount() < 1) {
        return Runtime::Value::string(new Runtime::String(str));
    }
    
    size_t targetLength = static_cast<size_t>(info.argument(0).asNumber());
    std::string padStr = " ";
    
    if (info.argumentCount() > 1) {
        padStr = info.argument(1).toString();
    }
    
    if (str.length() >= targetLength || padStr.empty()) {
        return Runtime::Value::string(new Runtime::String(str));
    }
    
    size_t padLen = targetLength - str.length();
    std::string padding;
    while (padding.length() < padLen) {
        padding += padStr;
    }
    padding = padding.substr(0, padLen);
    
    return Runtime::Value::string(new Runtime::String(padding + str));
}

// String.prototype.padEnd(targetLength, padString?)
Runtime::Value StringBuiltin::padEnd(const Runtime::FunctionCallInfo& info) {
    Runtime::Value thisVal = info.thisValue();
    std::string str = thisVal.toString();
    
    if (info.argumentCount() < 1) {
        return Runtime::Value::string(new Runtime::String(str));
    }
    
    size_t targetLength = static_cast<size_t>(info.argument(0).asNumber());
    std::string padStr = " ";
    
    if (info.argumentCount() > 1) {
        padStr = info.argument(1).toString();
    }
    
    if (str.length() >= targetLength || padStr.empty()) {
        return Runtime::Value::string(new Runtime::String(str));
    }
    
    size_t padLen = targetLength - str.length();
    std::string padding;
    while (padding.length() < padLen) {
        padding += padStr;
    }
    padding = padding.substr(0, padLen);
    
    return Runtime::Value::string(new Runtime::String(str + padding));
}

// String.prototype.replace(searchValue, replaceValue)
Runtime::Value StringBuiltin::replace(const Runtime::FunctionCallInfo& info) {
    Runtime::Value thisVal = info.thisValue();
    std::string str = thisVal.toString();
    
    if (info.argumentCount() < 2) {
        return Runtime::Value::string(new Runtime::String(str));
    }
    
    std::string search = info.argument(0).toString();
    std::string replacement = info.argument(1).toString();
    
    size_t pos = str.find(search);
    if (pos != std::string::npos) {
        str.replace(pos, search.length(), replacement);
    }
    
    return Runtime::Value::string(new Runtime::String(str));
}

// String.prototype.replaceAll(searchValue, replaceValue)
Runtime::Value StringBuiltin::replaceAll(const Runtime::FunctionCallInfo& info) {
    Runtime::Value thisVal = info.thisValue();
    std::string str = thisVal.toString();
    
    if (info.argumentCount() < 2) {
        return Runtime::Value::string(new Runtime::String(str));
    }
    
    std::string search = info.argument(0).toString();
    std::string replacement = info.argument(1).toString();
    
    if (search.empty()) {
        return Runtime::Value::string(new Runtime::String(str));
    }
    
    size_t pos = 0;
    while ((pos = str.find(search, pos)) != std::string::npos) {
        str.replace(pos, search.length(), replacement);
        pos += replacement.length();
    }
    
    return Runtime::Value::string(new Runtime::String(str));
}

// length getter
Runtime::Value StringBuiltin::getLength(const Runtime::FunctionCallInfo& info) {
    Runtime::Value thisVal = info.thisValue();
    std::string str = thisVal.toString();
    return Runtime::Value::number(static_cast<double>(str.length()));
}

// Create String prototype
Runtime::Object* StringBuiltin::createStringPrototype(Runtime::Context*) {
    Runtime::Object* prototype = new Runtime::Object();
    
    prototype->set("charAt", Runtime::Value::object(
        new Runtime::Function("charAt", charAt, 1)));
    prototype->set("charCodeAt", Runtime::Value::object(
        new Runtime::Function("charCodeAt", charCodeAt, 1)));
    prototype->set("concat", Runtime::Value::object(
        new Runtime::Function("concat", concat, 1)));
    prototype->set("includes", Runtime::Value::object(
        new Runtime::Function("includes", includes, 1)));
    prototype->set("indexOf", Runtime::Value::object(
        new Runtime::Function("indexOf", indexOf, 1)));
    prototype->set("lastIndexOf", Runtime::Value::object(
        new Runtime::Function("lastIndexOf", lastIndexOf, 1)));
    prototype->set("slice", Runtime::Value::object(
        new Runtime::Function("slice", slice, 2)));
    prototype->set("substring", Runtime::Value::object(
        new Runtime::Function("substring", substring, 2)));
    prototype->set("toLowerCase", Runtime::Value::object(
        new Runtime::Function("toLowerCase", toLowerCase, 0)));
    prototype->set("toUpperCase", Runtime::Value::object(
        new Runtime::Function("toUpperCase", toUpperCase, 0)));
    prototype->set("trim", Runtime::Value::object(
        new Runtime::Function("trim", trim, 0)));
    prototype->set("trimStart", Runtime::Value::object(
        new Runtime::Function("trimStart", trimStart, 0)));
    prototype->set("trimEnd", Runtime::Value::object(
        new Runtime::Function("trimEnd", trimEnd, 0)));
    prototype->set("split", Runtime::Value::object(
        new Runtime::Function("split", split, 2)));
    prototype->set("repeat", Runtime::Value::object(
        new Runtime::Function("repeat", repeat, 1)));
    prototype->set("startsWith", Runtime::Value::object(
        new Runtime::Function("startsWith", startsWith, 1)));
    prototype->set("endsWith", Runtime::Value::object(
        new Runtime::Function("endsWith", endsWith, 1)));
    prototype->set("padStart", Runtime::Value::object(
        new Runtime::Function("padStart", padStart, 2)));
    prototype->set("padEnd", Runtime::Value::object(
        new Runtime::Function("padEnd", padEnd, 2)));
    prototype->set("replace", Runtime::Value::object(
        new Runtime::Function("replace", replace, 2)));
    prototype->set("replaceAll", Runtime::Value::object(
        new Runtime::Function("replaceAll", replaceAll, 2)));
    
    return prototype;
}

} // namespace Zepra::Builtins
