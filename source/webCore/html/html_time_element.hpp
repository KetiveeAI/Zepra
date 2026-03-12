/**
 * @file html_time_element.hpp
 * @brief HTMLTimeElement - Machine-readable date/time
 *
 * Implements the <time> element per HTML Living Standard.
 * Represents dates, times, and durations in machine-readable format.
 *
 * @see https://html.spec.whatwg.org/multipage/text-level-semantics.html#the-time-element
 */

#pragma once

#include "html/html_element.hpp"
#include <string>
#include <memory>
#include <chrono>
#include <optional>

namespace Zepra::WebCore {

/**
 * @brief HTMLTimeElement - date/time
 *
 * The <time> element represents a specific period in time.
 * The datetime attribute provides machine-readable value.
 */
class HTMLTimeElement : public HTMLElement {
public:
    HTMLTimeElement();
    ~HTMLTimeElement() override;

    // =========================================================================
    // Core Attribute
    // =========================================================================

    /// Machine-readable date/time value
    std::string dateTime() const;
    void setDateTime(const std::string& datetime);

    // =========================================================================
    // Convenience Methods
    // =========================================================================

    /// Get the effective datetime (from attribute or text content)
    std::string effectiveDateTime() const;

    /// Parse as date (YYYY-MM-DD)
    std::optional<std::chrono::system_clock::time_point> asDate() const;

    /// Parse as time (HH:MM or HH:MM:SS)
    std::optional<std::chrono::seconds> asTime() const;

    /// Parse as datetime
    std::optional<std::chrono::system_clock::time_point> asDateTime() const;

    /// Parse as duration (ISO 8601 duration)
    std::optional<std::chrono::seconds> asDuration() const;

    /// Whether this represents a date
    bool isDate() const;

    /// Whether this represents a time
    bool isTime() const;

    /// Whether this represents a datetime
    bool isDateTime() const;

    /// Whether this represents a duration
    bool isDuration() const;

    // =========================================================================
    // Clone
    // =========================================================================

    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace Zepra::WebCore
