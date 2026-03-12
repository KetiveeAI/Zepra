// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file container.h
 * @brief Container widget for grouping children
 */

#pragma once

#include "widget.h"
#include "../layout/flexbox.h"

namespace NXRender {

/**
 * @brief Container widget that can hold children with layout
 */
class Container : public Widget {
public:
    Container();
    ~Container() override;
    
    // Layout
    FlexLayout& flexLayout() { return layout_; }
    void setFlexLayout(const FlexLayout& layout) { layout_ = layout; }
    
    // Rendering
    void render(GpuContext* ctx) override;
    Size measure(const Size& available) override;
    void layout() override;
    
private:
    FlexLayout layout_;
};

} // namespace NXRender
