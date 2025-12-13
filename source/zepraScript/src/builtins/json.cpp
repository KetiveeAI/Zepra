/**
 * @file json.cpp
 * @brief JSON builtin implementation (JSON.parse, JSON.stringify)
 */

#include "zeprascript/builtins/json.hpp"
#include "zeprascript/runtime/object.hpp"
#include "zeprascript/runtime/function.hpp"
#include "zeprascript/runtime/value.hpp"
#include <sstream>
#include <iomanip>
#include <cctype>
#include <stack>
#include <stdexcept>

namespace Zepra::Builtins {

// ============================================================================
// JSON Parser (Tokenizer + Recursive Descent)
// ============================================================================

enum class JsonTokenType {
    LeftBrace,    // {
    RightBrace,   // }
    LeftBracket,  // [
    RightBracket, // ]
    Colon,        // :
    Comma,        // ,
    String,
    Number,
    True,
    False,
    Null,
    EndOfInput,
    Error
};

struct JsonToken {
    JsonTokenType type;
    std::string value;
    double numValue = 0.0;
};

class JsonLexer {
public:
    explicit JsonLexer(const std::string& input) : input_(input), pos_(0) {}
    
    JsonToken nextToken() {
        skipWhitespace();
        
        if (pos_ >= input_.size()) {
            return {JsonTokenType::EndOfInput, ""};
        }
        
        char c = input_[pos_];
        
        switch (c) {
            case '{': pos_++; return {JsonTokenType::LeftBrace, "{"};
            case '}': pos_++; return {JsonTokenType::RightBrace, "}"};
            case '[': pos_++; return {JsonTokenType::LeftBracket, "["};
            case ']': pos_++; return {JsonTokenType::RightBracket, "]"};
            case ':': pos_++; return {JsonTokenType::Colon, ":"};
            case ',': pos_++; return {JsonTokenType::Comma, ","};
            case '"': return scanString();
            case 't': return scanKeyword("true", JsonTokenType::True);
            case 'f': return scanKeyword("false", JsonTokenType::False);
            case 'n': return scanKeyword("null", JsonTokenType::Null);
            default:
                if (c == '-' || std::isdigit(c)) {
                    return scanNumber();
                }
                return {JsonTokenType::Error, std::string("Unexpected character: ") + c};
        }
    }
    
private:
    void skipWhitespace() {
        while (pos_ < input_.size() && std::isspace(input_[pos_])) {
            pos_++;
        }
    }
    
    JsonToken scanString() {
        pos_++; // Skip opening quote
        std::string result;
        
        while (pos_ < input_.size() && input_[pos_] != '"') {
            if (input_[pos_] == '\\') {
                pos_++;
                if (pos_ >= input_.size()) break;
                
                switch (input_[pos_]) {
                    case '"': result += '"'; break;
                    case '\\': result += '\\'; break;
                    case '/': result += '/'; break;
                    case 'b': result += '\b'; break;
                    case 'f': result += '\f'; break;
                    case 'n': result += '\n'; break;
                    case 'r': result += '\r'; break;
                    case 't': result += '\t'; break;
                    case 'u': {
                        // Unicode escape
                        if (pos_ + 4 >= input_.size()) {
                            return {JsonTokenType::Error, "Invalid unicode escape"};
                        }
                        std::string hex = input_.substr(pos_ + 1, 4);
                        int codePoint = std::stoi(hex, nullptr, 16);
                        if (codePoint < 128) {
                            result += static_cast<char>(codePoint);
                        } else {
                            // Basic UTF-8 encoding for common characters
                            result += static_cast<char>(codePoint);
                        }
                        pos_ += 4;
                        break;
                    }
                    default:
                        result += input_[pos_];
                }
            } else {
                result += input_[pos_];
            }
            pos_++;
        }
        
        if (pos_ >= input_.size()) {
            return {JsonTokenType::Error, "Unterminated string"};
        }
        
        pos_++; // Skip closing quote
        return {JsonTokenType::String, result};
    }
    
    JsonToken scanNumber() {
        size_t start = pos_;
        
        if (input_[pos_] == '-') pos_++;
        
        while (pos_ < input_.size() && std::isdigit(input_[pos_])) {
            pos_++;
        }
        
        if (pos_ < input_.size() && input_[pos_] == '.') {
            pos_++;
            while (pos_ < input_.size() && std::isdigit(input_[pos_])) {
                pos_++;
            }
        }
        
        if (pos_ < input_.size() && (input_[pos_] == 'e' || input_[pos_] == 'E')) {
            pos_++;
            if (pos_ < input_.size() && (input_[pos_] == '+' || input_[pos_] == '-')) {
                pos_++;
            }
            while (pos_ < input_.size() && std::isdigit(input_[pos_])) {
                pos_++;
            }
        }
        
        std::string numStr = input_.substr(start, pos_ - start);
        JsonToken token{JsonTokenType::Number, numStr};
        token.numValue = std::stod(numStr);
        return token;
    }
    
    JsonToken scanKeyword(const std::string& keyword, JsonTokenType type) {
        if (input_.substr(pos_, keyword.size()) == keyword) {
            pos_ += keyword.size();
            return {type, keyword};
        }
        return {JsonTokenType::Error, "Invalid keyword"};
    }
    
    std::string input_;
    size_t pos_;
};

class JsonParser {
public:
    explicit JsonParser(const std::string& input) : lexer_(input) {
        advance();
    }
    
    Runtime::Value parse() {
        Runtime::Value result = parseValue();
        if (current_.type != JsonTokenType::EndOfInput) {
            throw std::runtime_error("Unexpected token after JSON value");
        }
        return result;
    }
    
private:
    void advance() {
        current_ = lexer_.nextToken();
        if (current_.type == JsonTokenType::Error) {
            throw std::runtime_error(current_.value);
        }
    }
    
    Runtime::Value parseValue() {
        switch (current_.type) {
            case JsonTokenType::LeftBrace:
                return parseObject();
            case JsonTokenType::LeftBracket:
                return parseArray();
            case JsonTokenType::String:
                return parseString();
            case JsonTokenType::Number:
                return parseNumber();
            case JsonTokenType::True:
                advance();
                return Runtime::Value::boolean(true);
            case JsonTokenType::False:
                advance();
                return Runtime::Value::boolean(false);
            case JsonTokenType::Null:
                advance();
                return Runtime::Value::null();
            default:
                throw std::runtime_error("Unexpected token in JSON");
        }
    }
    
    Runtime::Value parseObject() {
        advance(); // Skip {
        
        Runtime::Object* obj = new Runtime::Object();
        
        if (current_.type == JsonTokenType::RightBrace) {
            advance();
            return Runtime::Value::object(obj);
        }
        
        while (true) {
            if (current_.type != JsonTokenType::String) {
                throw std::runtime_error("Expected string key in object");
            }
            std::string key = current_.value;
            advance();
            
            if (current_.type != JsonTokenType::Colon) {
                throw std::runtime_error("Expected ':' after key");
            }
            advance();
            
            Runtime::Value value = parseValue();
            obj->set(key, value);
            
            if (current_.type == JsonTokenType::RightBrace) {
                advance();
                break;
            }
            
            if (current_.type != JsonTokenType::Comma) {
                throw std::runtime_error("Expected ',' or '}' in object");
            }
            advance();
        }
        
        return Runtime::Value::object(obj);
    }
    
    Runtime::Value parseArray() {
        advance(); // Skip [
        
        std::vector<Runtime::Value> elements;
        
        if (current_.type == JsonTokenType::RightBracket) {
            advance();
            return Runtime::Value::object(new Runtime::Array(std::move(elements)));
        }
        
        while (true) {
            elements.push_back(parseValue());
            
            if (current_.type == JsonTokenType::RightBracket) {
                advance();
                break;
            }
            
            if (current_.type != JsonTokenType::Comma) {
                throw std::runtime_error("Expected ',' or ']' in array");
            }
            advance();
        }
        
        return Runtime::Value::object(new Runtime::Array(std::move(elements)));
    }
    
    Runtime::Value parseString() {
        std::string value = current_.value;
        advance();
        return Runtime::Value::string(new Runtime::String(value));
    }
    
    Runtime::Value parseNumber() {
        double value = current_.numValue;
        advance();
        return Runtime::Value::number(value);
    }
    
    JsonLexer lexer_;
    JsonToken current_;
};

// ============================================================================
// JSON.stringify implementation
// ============================================================================

class JsonStringifier {
public:
    explicit JsonStringifier(int indent = 0) : indent_(indent), depth_(0) {}
    
    std::string stringify(Runtime::Value value) {
        return stringifyValue(value);
    }
    
private:
    std::string stringifyValue(Runtime::Value value) {
        if (value.isUndefined()) {
            return "null"; // undefined becomes null in JSON
        }
        
        if (value.isNull()) {
            return "null";
        }
        
        if (value.isBoolean()) {
            return value.asBoolean() ? "true" : "false";
        }
        
        if (value.isNumber()) {
            double num = value.asNumber();
            if (std::isnan(num) || std::isinf(num)) {
                return "null"; // NaN/Infinity become null
            }
            std::ostringstream ss;
            ss << std::setprecision(17) << num;
            return ss.str();
        }
        
        if (value.isString()) {
            return stringifyString(value.toString());
        }
        
        if (value.isObject()) {
            Runtime::Object* obj = value.asObject();
            
            if (auto* arr = dynamic_cast<Runtime::Array*>(obj)) {
                return stringifyArray(arr);
            }
            
            if (obj->isFunction()) {
                return ""; // Functions become empty in JSON
            }
            
            return stringifyObject(obj);
        }
        
        return "null";
    }
    
    std::string stringifyString(const std::string& str) {
        std::ostringstream ss;
        ss << '"';
        
        for (char c : str) {
            switch (c) {
                case '"': ss << "\\\""; break;
                case '\\': ss << "\\\\"; break;
                case '\b': ss << "\\b"; break;
                case '\f': ss << "\\f"; break;
                case '\n': ss << "\\n"; break;
                case '\r': ss << "\\r"; break;
                case '\t': ss << "\\t"; break;
                default:
                    if (static_cast<unsigned char>(c) < 32) {
                        ss << "\\u" << std::hex << std::setw(4) 
                           << std::setfill('0') << static_cast<int>(c);
                    } else {
                        ss << c;
                    }
            }
        }
        
        ss << '"';
        return ss.str();
    }
    
    std::string stringifyArray(Runtime::Array* arr) {
        if (arr->length() == 0) {
            return "[]";
        }
        
        depth_++;
        std::ostringstream ss;
        ss << '[';
        
        for (size_t i = 0; i < arr->length(); i++) {
            if (i > 0) ss << ',';
            if (indent_ > 0) {
                ss << '\n' << std::string(depth_ * indent_, ' ');
            }
            
            std::string val = stringifyValue(arr->at(i));
            ss << (val.empty() ? "null" : val);
        }
        
        depth_--;
        if (indent_ > 0) {
            ss << '\n' << std::string(depth_ * indent_, ' ');
        }
        ss << ']';
        
        return ss.str();
    }
    
    std::string stringifyObject(Runtime::Object* obj) {
        auto keys = obj->keys();
        if (keys.empty()) {
            return "{}";
        }
        
        depth_++;
        std::ostringstream ss;
        ss << '{';
        
        bool first = true;
        for (const auto& key : keys) {
            Runtime::Value val = obj->get(key);
            std::string valStr = stringifyValue(val);
            
            if (valStr.empty()) continue; // Skip undefined/function properties
            
            if (!first) ss << ',';
            first = false;
            
            if (indent_ > 0) {
                ss << '\n' << std::string(depth_ * indent_, ' ');
            }
            
            ss << stringifyString(key) << ':';
            if (indent_ > 0) ss << ' ';
            ss << valStr;
        }
        
        depth_--;
        if (indent_ > 0 && !keys.empty()) {
            ss << '\n' << std::string(depth_ * indent_, ' ');
        }
        ss << '}';
        
        return ss.str();
    }
    
    int indent_;
    int depth_;
};

// ============================================================================
// JSON builtin methods
// ============================================================================

Runtime::Value JSONBuiltin::parse(const Runtime::FunctionCallInfo& info) {
    if (info.argumentCount() < 1) {
        throw std::runtime_error("JSON.parse requires a string argument");
    }
    
    std::string jsonStr = info.argument(0).toString();
    
    try {
        JsonParser parser(jsonStr);
        return parser.parse();
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("SyntaxError: ") + e.what());
    }
}

Runtime::Value JSONBuiltin::stringify(const Runtime::FunctionCallInfo& info) {
    if (info.argumentCount() < 1) {
        return Runtime::Value::undefined();
    }
    
    Runtime::Value value = info.argument(0);
    
    // Get indent (third argument)
    int indent = 0;
    if (info.argumentCount() >= 3) {
        if (info.argument(2).isNumber()) {
            indent = std::min(10, static_cast<int>(info.argument(2).asNumber()));
            indent = std::max(0, indent);
        }
    }
    
    try {
        JsonStringifier stringifier(indent);
        std::string result = stringifier.stringify(value);
        
        if (result.empty()) {
            return Runtime::Value::undefined();
        }
        
        return Runtime::Value::string(new Runtime::String(result));
    } catch (const std::exception&) {
        return Runtime::Value::undefined();
    }
}

Runtime::Object* JSONBuiltin::createJSONObject(Runtime::Context*) {
    Runtime::Object* json = new Runtime::Object();
    
    json->set("parse", Runtime::Value::object(
        new Runtime::Function("parse", parse, 1)));
    json->set("stringify", Runtime::Value::object(
        new Runtime::Function("stringify", stringify, 3)));
    
    return json;
}

} // namespace Zepra::Builtins
