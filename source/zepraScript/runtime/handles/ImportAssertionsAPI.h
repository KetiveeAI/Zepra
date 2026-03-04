/**
 * @file ImportAssertionsAPI.h
 * @brief Import Assertions/Attributes Implementation
 */

#pragma once

#include <string>
#include <map>
#include <optional>
#include <functional>
#include <stdexcept>

namespace Zepra::Runtime {

// =============================================================================
// Module Types
// =============================================================================

enum class ModuleType {
    JavaScript,
    JSON,
    CSS,
    WebAssembly,
    Unknown
};

inline std::string moduleTypeToString(ModuleType type) {
    switch (type) {
        case ModuleType::JavaScript: return "javascript";
        case ModuleType::JSON: return "json";
        case ModuleType::CSS: return "css";
        case ModuleType::WebAssembly: return "webassembly";
        default: return "unknown";
    }
}

inline ModuleType stringToModuleType(const std::string& str) {
    if (str == "json") return ModuleType::JSON;
    if (str == "css") return ModuleType::CSS;
    if (str == "webassembly" || str == "wasm") return ModuleType::WebAssembly;
    if (str == "javascript" || str == "js") return ModuleType::JavaScript;
    return ModuleType::Unknown;
}

// =============================================================================
// Import Attributes
// =============================================================================

class ImportAttributes {
public:
    ImportAttributes() = default;
    
    void set(const std::string& key, const std::string& value) {
        attributes_[key] = value;
    }
    
    std::optional<std::string> get(const std::string& key) const {
        auto it = attributes_.find(key);
        return it != attributes_.end() ? std::optional(it->second) : std::nullopt;
    }
    
    bool has(const std::string& key) const {
        return attributes_.find(key) != attributes_.end();
    }
    
    ModuleType type() const {
        auto t = get("type");
        return t ? stringToModuleType(*t) : ModuleType::JavaScript;
    }
    
    const std::map<std::string, std::string>& all() const { return attributes_; }
    
    bool empty() const { return attributes_.empty(); }

private:
    std::map<std::string, std::string> attributes_;
};

// =============================================================================
// Import Specifier
// =============================================================================

struct ImportSpecifier {
    std::string specifier;
    ImportAttributes attributes;
    
    ModuleType expectedType() const { return attributes.type(); }
};

// =============================================================================
// Module Validator
// =============================================================================

class ModuleTypeValidator {
public:
    static bool validate(const std::string& content, ModuleType expectedType) {
        switch (expectedType) {
            case ModuleType::JSON:
                return isValidJSON(content);
            case ModuleType::CSS:
                return isValidCSS(content);
            case ModuleType::WebAssembly:
                return isValidWasm(content);
            default:
                return true;
        }
    }

private:
    static bool isValidJSON(const std::string& content) {
        if (content.empty()) return false;
        char first = content[0];
        while (std::isspace(first) && &first < &content.back()) first = *(&first + 1);
        return first == '{' || first == '[' || first == '"' || 
               first == 't' || first == 'f' || first == 'n' ||
               std::isdigit(first) || first == '-';
    }
    
    static bool isValidCSS(const std::string& content) {
        return content.find('{') != std::string::npos;
    }
    
    static bool isValidWasm(const std::string& content) {
        return content.size() >= 4 && 
               content[0] == 0x00 && content[1] == 0x61 && 
               content[2] == 0x73 && content[3] == 0x6d;
    }
};

// =============================================================================
// JSON Module
// =============================================================================

class JSONModule {
public:
    explicit JSONModule(const std::string& content) : content_(content) {}
    
    const std::string& content() const { return content_; }
    
    template<typename T>
    T parse() const;

private:
    std::string content_;
};

// =============================================================================
// CSS Module
// =============================================================================

class CSSModule {
public:
    explicit CSSModule(const std::string& content) : content_(content) {}
    
    const std::string& content() const { return content_; }
    
    std::string adoptedStyleSheet() const { return content_; }

private:
    std::string content_;
};

// =============================================================================
// Import Assertion Error
// =============================================================================

class ImportAssertionError : public std::runtime_error {
public:
    ImportAssertionError(const std::string& specifier, const std::string& reason)
        : std::runtime_error("Import assertion failed for '" + specifier + "': " + reason)
        , specifier_(specifier), reason_(reason) {}
    
    const std::string& specifier() const { return specifier_; }
    const std::string& reason() const { return reason_; }

private:
    std::string specifier_;
    std::string reason_;
};

} // namespace Zepra::Runtime
