/**
 * @file html_progress_element.hpp
 * @brief HTMLProgressElement - Progress indicator
 *
 * Implements the <progress> element per HTML Living Standard.
 * Shows completion progress of a task.
 *
 * @see https://html.spec.whatwg.org/multipage/form-elements.html#the-progress-element
 */

#pragma once

#include "html/html_element.hpp"
#include <string>
#include <memory>
#include <vector>

namespace Zepra::WebCore {

/**
 * @brief HTMLProgressElement - progress indicator
 *
 * The <progress> element represents the completion progress of a task.
 * It can be determinate (with value) or indeterminate (without value).
 */
class HTMLProgressElement : public HTMLElement {
public:
    HTMLProgressElement();
    ~HTMLProgressElement() override;

    // =========================================================================
    // Core Attributes
    // =========================================================================

    /// Current progress value
    double value() const;
    void setValue(double value);

    /// Maximum value (default 1.0)
    double max() const;
    void setMax(double max);

    // =========================================================================
    // Computed Properties
    // =========================================================================

    /// Progress position (value/max, or -1 if indeterminate)
    double position() const;

    /// Whether this is an indeterminate progress (no value set)
    bool isIndeterminate() const;

    /// Labels associated with this progress element
    std::vector<HTMLElement*> labels() const;

    // =========================================================================
    // Clone
    // =========================================================================

    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace Zepra::WebCore
