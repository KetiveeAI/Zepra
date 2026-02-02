/**
 * @file JSONAPI.h
 * @brief JSON Implementation
 */

#pragma once

#include <string>
#include <variant>
#include <vector>
#include <unordered_map>
#include <memory>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <functional>
#include <unordered_set>

namespace Zepra::Runtime {

class JSONValue;
using JSONArray = std::vector<std::shared_ptr<JSONValue>>;
using JSONObject = std::unordered_map<std::string, std::shared_ptr<JSONValue>>;

class JSONValue {
public:
    using Value = std::variant<std::nullptr_t, bool, double, std::string, JSONArray, JSONObject>;
    
    JSONValue() : value_(nullptr) {}
    explicit JSONValue(std::nullptr_t) : value_(nullptr) {}
    explicit JSONValue(bool b) : value_(b) {}
    explicit JSONValue(double n) : value_(n) {}
    explicit JSONValue(const std::string& s) : value_(s) {}
    explicit JSONValue(const JSONArray& arr) : value_(arr) {}
    explicit JSONValue(const JSONObject& obj) : value_(obj) {}
    
    bool isNull() const { return std::holds_alternative<std::nullptr_t>(value_); }
    bool isBool() const { return std::holds_alternative<bool>(value_); }
    bool isNumber() const { return std::holds_alternative<double>(value_); }
    bool isString() const { return std::holds_alternative<std::string>(value_); }
    bool isArray() const { return std::holds_alternative<JSONArray>(value_); }
    bool isObject() const { return std::holds_alternative<JSONObject>(value_); }
    
    bool asBool() const { return std::get<bool>(value_); }
    double asNumber() const { return std::get<double>(value_); }
    const std::string& asString() const { return std::get<std::string>(value_); }
    const JSONArray& asArray() const { return std::get<JSONArray>(value_); }
    const JSONObject& asObject() const { return std::get<JSONObject>(value_); }

private:
    Value value_;
};

class JSONParser {
public:
    explicit JSONParser(const std::string& text) : text_(text), pos_(0) {}
    
    std::shared_ptr<JSONValue> parse() {
        skipWhitespace();
        return parseValue();
    }

private:
    std::shared_ptr<JSONValue> parseValue() {
        skipWhitespace();
        if (pos_ >= text_.length()) return nullptr;
        
        char c = text_[pos_];
        if (c == '{') return parseObject();
        if (c == '[') return parseArray();
        if (c == '"') return parseString();
        if (c == 't') { pos_ += 4; return std::make_shared<JSONValue>(true); }
        if (c == 'f') { pos_ += 5; return std::make_shared<JSONValue>(false); }
        if (c == 'n') { pos_ += 4; return std::make_shared<JSONValue>(nullptr); }
        if (c == '-' || (c >= '0' && c <= '9')) return parseNumber();
        return nullptr;
    }
    
    std::shared_ptr<JSONValue> parseObject() {
        ++pos_;
        skipWhitespace();
        JSONObject obj;
        if (text_[pos_] != '}') {
            while (true) {
                skipWhitespace();
                std::string key = parseStringValue();
                skipWhitespace();
                ++pos_; // :
                obj[key] = parseValue();
                skipWhitespace();
                if (text_[pos_] == '}') break;
                ++pos_; // ,
            }
        }
        ++pos_;
        return std::make_shared<JSONValue>(obj);
    }
    
    std::shared_ptr<JSONValue> parseArray() {
        ++pos_;
        skipWhitespace();
        JSONArray arr;
        if (text_[pos_] != ']') {
            while (true) {
                arr.push_back(parseValue());
                skipWhitespace();
                if (text_[pos_] == ']') break;
                ++pos_; // ,
            }
        }
        ++pos_;
        return std::make_shared<JSONValue>(arr);
    }
    
    std::shared_ptr<JSONValue> parseString() {
        return std::make_shared<JSONValue>(parseStringValue());
    }
    
    std::string parseStringValue() {
        ++pos_; // "
        std::string result;
        while (pos_ < text_.length() && text_[pos_] != '"') {
            if (text_[pos_] == '\\') {
                ++pos_;
                switch (text_[pos_]) {
                    case 'n': result += '\n'; break;
                    case 't': result += '\t'; break;
                    case 'r': result += '\r'; break;
                    case '"': result += '"'; break;
                    case '\\': result += '\\'; break;
                    default: result += text_[pos_];
                }
            } else {
                result += text_[pos_];
            }
            ++pos_;
        }
        ++pos_; // "
        return result;
    }
    
    std::shared_ptr<JSONValue> parseNumber() {
        size_t start = pos_;
        if (text_[pos_] == '-') ++pos_;
        while (pos_ < text_.length() && (text_[pos_] >= '0' && text_[pos_] <= '9')) ++pos_;
        if (pos_ < text_.length() && text_[pos_] == '.') {
            ++pos_;
            while (pos_ < text_.length() && (text_[pos_] >= '0' && text_[pos_] <= '9')) ++pos_;
        }
        if (pos_ < text_.length() && (text_[pos_] == 'e' || text_[pos_] == 'E')) {
            ++pos_;
            if (text_[pos_] == '+' || text_[pos_] == '-') ++pos_;
            while (pos_ < text_.length() && (text_[pos_] >= '0' && text_[pos_] <= '9')) ++pos_;
        }
        return std::make_shared<JSONValue>(std::stod(text_.substr(start, pos_ - start)));
    }
    
    void skipWhitespace() {
        while (pos_ < text_.length() && (text_[pos_] == ' ' || text_[pos_] == '\t' || 
               text_[pos_] == '\n' || text_[pos_] == '\r')) ++pos_;
    }
    
    std::string text_;
    size_t pos_;
};

class JSONStringifier {
public:
    std::string stringify(const std::shared_ptr<JSONValue>& value, int indent = 0) {
        indent_ = indent;
        return stringifyValue(value, 0);
    }

private:
    std::string stringifyValue(const std::shared_ptr<JSONValue>& v, int depth) {
        if (!v || v->isNull()) return "null";
        if (v->isBool()) return v->asBool() ? "true" : "false";
        if (v->isNumber()) {
            double n = v->asNumber();
            if (std::isnan(n) || std::isinf(n)) return "null";
            std::ostringstream oss;
            oss << std::setprecision(17) << n;
            return oss.str();
        }
        if (v->isString()) return escapeString(v->asString());
        if (v->isArray()) return stringifyArray(v, depth);
        if (v->isObject()) return stringifyObject(v, depth);
        return "null";
    }
    
    std::string stringifyArray(const std::shared_ptr<JSONValue>& arr, int depth) {
        if (arr->asArray().empty()) return "[]";
        std::string result = "[";
        for (size_t i = 0; i < arr->asArray().size(); ++i) {
            if (i > 0) result += ",";
            if (indent_ > 0) result += "\n" + std::string((depth + 1) * indent_, ' ');
            result += stringifyValue(arr->asArray()[i], depth + 1);
        }
        if (indent_ > 0) result += "\n" + std::string(depth * indent_, ' ');
        return result + "]";
    }
    
    std::string stringifyObject(const std::shared_ptr<JSONValue>& obj, int depth) {
        if (obj->asObject().empty()) return "{}";
        std::string result = "{";
        bool first = true;
        for (const auto& [key, value] : obj->asObject()) {
            if (!first) result += ",";
            first = false;
            if (indent_ > 0) result += "\n" + std::string((depth + 1) * indent_, ' ');
            result += escapeString(key) + (indent_ > 0 ? ": " : ":") + stringifyValue(value, depth + 1);
        }
        if (indent_ > 0) result += "\n" + std::string(depth * indent_, ' ');
        return result + "}";
    }
    
    std::string escapeString(const std::string& s) {
        std::string result = "\"";
        for (char c : s) {
            switch (c) {
                case '"': result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default: result += c;
            }
        }
        return result + "\"";
    }
    
    int indent_ = 0;
};

inline std::shared_ptr<JSONValue> jsonParse(const std::string& text) {
    return JSONParser(text).parse();
}

inline std::string jsonStringify(const std::shared_ptr<JSONValue>& value, int indent = 0) {
    return JSONStringifier().stringify(value, indent);
}

} // namespace Zepra::Runtime
