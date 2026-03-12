/**
 * @file html_element_factory.hpp
 * @brief Factory for creating HTML elements by tag name
 */

#pragma once

#include "html/html_element.hpp"
#include <memory>
#include <string>
#include <functional>
#include <unordered_map>

namespace Zepra::WebCore {

/**
 * @brief Factory for creating HTML elements
 */
class HTMLElementFactory {
public:
    using Creator = std::function<std::unique_ptr<HTMLElement>()>;
    
    static HTMLElementFactory& instance();
    
    /**
     * @brief Create an element by tag name
     */
    std::unique_ptr<HTMLElement> createElement(const std::string& tagName) const;
    
    /**
     * @brief Register a custom element creator
     */
    void registerElement(const std::string& tagName, Creator creator);
    
    /**
     * @brief Check if a tag name is known
     */
    bool isKnownElement(const std::string& tagName) const;
    
    /**
     * @brief Get all registered tag names
     */
    std::vector<std::string> registeredElements() const;
    
private:
    HTMLElementFactory();
    ~HTMLElementFactory() = default;
    
    void registerStandardElements();
    
    std::unordered_map<std::string, Creator> creators_;
};

/**
 * @brief Helper macro to register element types
 */
#define REGISTER_HTML_ELEMENT(TagName, ClassName) \
    HTMLElementFactory::instance().registerElement(TagName, \
        []() -> std::unique_ptr<HTMLElement> { \
            return std::make_unique<ClassName>(); \
        })

} // namespace Zepra::WebCore
