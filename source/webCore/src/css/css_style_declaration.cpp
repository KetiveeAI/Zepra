/**
 * @file css_style_declaration.cpp
 * @brief CSSStyleDeclaration implementation
 */

#include "webcore/css/css_style_declaration.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>

namespace Zepra::WebCore {

CSSStyleDeclaration::CSSStyleDeclaration(CSSRule* parentRule)
    : parentRule_(parentRule) {}

std::string CSSStyleDeclaration::item(size_t index) const {
    if (index >= propertyOrder_.size()) return "";
    return propertyOrder_[index];
}

std::string CSSStyleDeclaration::getPropertyValue(const std::string& property) const {
    std::string normalized = normalizeProperty(property);
    auto it = properties_.find(normalized);
    if (it != properties_.end()) {
        return it->second.value;
    }
    return "";
}

std::string CSSStyleDeclaration::getPropertyPriority(const std::string& property) const {
    std::string normalized = normalizeProperty(property);
    auto it = properties_.find(normalized);
    if (it != properties_.end() && it->second.important) {
        return "important";
    }
    return "";
}

void CSSStyleDeclaration::setProperty(const std::string& property, const std::string& value,
                                       const std::string& priority) {
    if (value.empty()) {
        removeProperty(property);
        return;
    }
    
    std::string normalized = normalizeProperty(property);
    bool important = (priority == "important");
    
    auto it = properties_.find(normalized);
    if (it == properties_.end()) {
        propertyOrder_.push_back(normalized);
    }
    
    properties_[normalized] = {value, important};
}

std::string CSSStyleDeclaration::removeProperty(const std::string& property) {
    std::string normalized = normalizeProperty(property);
    auto it = properties_.find(normalized);
    if (it == properties_.end()) return "";
    
    std::string oldValue = it->second.value;
    properties_.erase(it);
    
    propertyOrder_.erase(
        std::remove(propertyOrder_.begin(), propertyOrder_.end(), normalized),
        propertyOrder_.end());
    
    return oldValue;
}

std::string CSSStyleDeclaration::cssText() const {
    std::ostringstream oss;
    
    for (const auto& prop : propertyOrder_) {
        auto it = properties_.find(prop);
        if (it != properties_.end()) {
            oss << prop << ": " << it->second.value;
            if (it->second.important) {
                oss << " !important";
            }
            oss << "; ";
        }
    }
    
    return oss.str();
}

void CSSStyleDeclaration::setCssText(const std::string& text) {
    properties_.clear();
    propertyOrder_.clear();
    
    // Simple parsing
    size_t pos = 0;
    while (pos < text.size()) {
        // Find property name
        size_t colonPos = text.find(':', pos);
        if (colonPos == std::string::npos) break;
        
        std::string property = text.substr(pos, colonPos - pos);
        
        // Trim
        size_t start = property.find_first_not_of(" \t\n\r");
        size_t end = property.find_last_not_of(" \t\n\r");
        if (start != std::string::npos) {
            property = property.substr(start, end - start + 1);
        }
        
        // Find value
        size_t semiPos = text.find(';', colonPos);
        if (semiPos == std::string::npos) semiPos = text.size();
        
        std::string value = text.substr(colonPos + 1, semiPos - colonPos - 1);
        
        // Check for !important
        bool important = false;
        size_t impPos = value.find("!important");
        if (impPos != std::string::npos) {
            important = true;
            value = value.substr(0, impPos);
        }
        
        // Trim value
        start = value.find_first_not_of(" \t\n\r");
        end = value.find_last_not_of(" \t\n\r");
        if (start != std::string::npos) {
            value = value.substr(start, end - start + 1);
        }
        
        if (!property.empty() && !value.empty()) {
            setProperty(property, value, important ? "important" : "");
        }
        
        pos = semiPos + 1;
    }
}

std::string CSSStyleDeclaration::normalizeProperty(const std::string& property) const {
    std::string result;
    result.reserve(property.size());
    
    for (char c : property) {
        result += std::tolower(static_cast<unsigned char>(c));
    }
    
    return result;
}

} // namespace Zepra::WebCore
