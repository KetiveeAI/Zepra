/**
 * @file html_datalist_element.hpp
 * @brief HTMLDataListElement - Input suggestions
 *
 * Implements the <datalist> element per HTML Living Standard.
 * Provides a list of suggested values for input controls.
 *
 * @see https://html.spec.whatwg.org/multipage/form-elements.html#the-datalist-element
 */

#pragma once

#include "html/html_element.hpp"
#include <string>
#include <memory>
#include <vector>

namespace Zepra::WebCore {

// Forward declarations
class HTMLOptionElement;
class HTMLCollection;

/**
 * @brief HTMLDataListElement - input suggestions
 *
 * The <datalist> element represents a set of option elements
 * that represent predefined options for other controls.
 */
class HTMLDataListElement : public HTMLElement {
public:
    HTMLDataListElement();
    ~HTMLDataListElement() override;

    // =========================================================================
    // Options Access
    // =========================================================================

    /// Collection of option elements
    HTMLCollection* options() const;

    /// Get all option elements as a vector
    std::vector<HTMLOptionElement*> optionElements() const;

    /// Number of options
    size_t length() const;

    /// Get option by index
    HTMLOptionElement* item(size_t index) const;

    /// Get option by id or name
    HTMLOptionElement* namedItem(const std::string& name) const;

    // =========================================================================
    // Convenience
    // =========================================================================

    /// Get all option values
    std::vector<std::string> values() const;

    /// Check if a value is in the options
    bool hasValue(const std::string& value) const;

    // =========================================================================
    // Clone
    // =========================================================================

    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace Zepra::WebCore
