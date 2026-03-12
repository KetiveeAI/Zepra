/**
 * @file html_custom_element.hpp
 * @brief Custom Elements API
 */

#pragma once

#include "html/html_element.hpp"
#include <string>
#include <functional>
#include <unordered_map>
#include <vector>

namespace Zepra::WebCore {

/**
 * @brief Custom element lifecycle callbacks
 */
struct CustomElementCallbacks {
    std::function<void(HTMLElement*)> connectedCallback;
    std::function<void(HTMLElement*)> disconnectedCallback;
    std::function<void(HTMLElement*, const std::string&, HTMLElement*)> adoptedCallback;
    std::function<void(HTMLElement*, const std::string&, const std::string&, const std::string&)> attributeChangedCallback;
};

/**
 * @brief Custom element definition
 */
struct CustomElementDefinition {
    std::string name;
    std::string extends;
    std::vector<std::string> observedAttributes;
    CustomElementCallbacks callbacks;
    std::function<std::unique_ptr<HTMLElement>()> constructor;
};

/**
 * @brief Custom Elements Registry
 */
class CustomElementRegistry {
public:
    static CustomElementRegistry& instance();
    
    /**
     * @brief Define a custom element
     */
    void define(const std::string& name, 
                std::function<std::unique_ptr<HTMLElement>()> constructor,
                const std::string& extends = "");
    
    /**
     * @brief Get a custom element definition
     */
    const CustomElementDefinition* get(const std::string& name) const;
    
    /**
     * @brief Check when a custom element is defined
     */
    void whenDefined(const std::string& name, std::function<void()> callback);
    
    /**
     * @brief Upgrade an element to a custom element
     */
    void upgrade(HTMLElement* element);
    
    /**
     * @brief Check if name is valid custom element name
     */
    static bool isValidName(const std::string& name);
    
private:
    CustomElementRegistry() = default;
    std::unordered_map<std::string, CustomElementDefinition> definitions_;
    std::unordered_map<std::string, std::vector<std::function<void()>>> pending_;
};

/**
 * @brief Autonomous custom element base
 */
class HTMLCustomElement : public HTMLElement {
public:
    HTMLCustomElement();
    ~HTMLCustomElement() override;
    
    void connectedCallback();
    void disconnectedCallback();
    void adoptedCallback(HTMLElement* oldDocument);
    void attributeChangedCallback(const std::string& name,
                                   const std::string& oldValue,
                                   const std::string& newValue);
    
    bool isCustomElement() const override { return true; }
    
protected:
    virtual void onConnected() {}
    virtual void onDisconnected() {}
    virtual void onAdopted(HTMLElement*) {}
    virtual void onAttributeChanged(const std::string&, const std::string&, const std::string&) {}
};

/**
 * @brief Customized built-in element
 */
template<typename Base>
class HTMLCustomizedBuiltInElement : public Base {
public:
    HTMLCustomizedBuiltInElement() = default;
    ~HTMLCustomizedBuiltInElement() override = default;
    
    bool isCustomElement() const override { return true; }
    
    std::string is() const { return is_; }
    void setIs(const std::string& name) { is_ = name; }
    
private:
    std::string is_;
};

/**
 * @brief Shadow root for web components
 */
class ShadowRoot : public HTMLElement {
public:
    enum class Mode { Open, Closed };
    
    explicit ShadowRoot(Mode mode);
    ~ShadowRoot() override = default;
    
    Mode mode() const { return mode_; }
    HTMLElement* host() const { return host_; }
    
    std::string innerHTML() const;
    void setInnerHTML(const std::string& html);
    
    bool delegatesFocus() const { return delegatesFocus_; }
    
    std::string tagName() const { return "#shadow-root"; }
    
private:
    Mode mode_;
    HTMLElement* host_ = nullptr;
    bool delegatesFocus_ = false;
    
    friend class HTMLElement;
};

} // namespace Zepra::WebCore
