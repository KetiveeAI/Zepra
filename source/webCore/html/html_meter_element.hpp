/**
 * @file html_meter_element.hpp
 * @brief HTMLMeterElement - Gauge/meter element
 *
 * Implements the <meter> element per HTML Living Standard.
 * Represents a scalar measurement within a known range.
 *
 * @see https://html.spec.whatwg.org/multipage/form-elements.html#the-meter-element
 */

#pragma once

#include "html/html_element.hpp"
#include <string>
#include <memory>
#include <vector>

namespace Zepra::WebCore {

/**
 * @brief HTMLMeterElement - scalar gauge
 *
 * The <meter> element represents a scalar measurement within a known range,
 * or a fractional value.
 */
class HTMLMeterElement : public HTMLElement {
public:
    HTMLMeterElement();
    ~HTMLMeterElement() override;

    // =========================================================================
    // Core Attributes
    // =========================================================================

    /// Current value
    double value() const;
    void setValue(double value);

    /// Minimum value (default 0)
    double min() const;
    void setMin(double min);

    /// Maximum value (default 1)
    double max() const;
    void setMax(double max);

    /// Low threshold (below this is "low")
    double low() const;
    void setLow(double low);

    /// High threshold (above this is "high")
    double high() const;
    void setHigh(double high);

    /// Optimum value (the "best" value)
    double optimum() const;
    void setOptimum(double optimum);

    // =========================================================================
    // Computed Properties
    // =========================================================================

    /// Labels associated with this meter
    std::vector<HTMLElement*> labels() const;

    /// Whether value is in the low range
    bool isLow() const;

    /// Whether value is in the high range
    bool isHigh() const;

    /// Whether value is at optimum
    bool isOptimum() const;

    // =========================================================================
    // Clone
    // =========================================================================

    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace Zepra::WebCore
