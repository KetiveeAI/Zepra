// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#pragma once

#include "widgets/widget.h"
#include <string>
#include <vector>
#include <functional>

namespace NXRender {
namespace Widgets {

struct MenuItem {
    std::string text;
    std::string shortcut;
    bool isSeparator = false;
    bool enabled = true;
    std::function<void()> onTriggered;
    std::vector<MenuItem> subMenuItems;
};

class MenuWidget : public Widget {
public:
    MenuWidget();
    ~MenuWidget() override;

    void addItem(const MenuItem& item);
    void addSeparator();

    void render(GpuContext* ctx) override;
    EventResult handleRoutedEvent(const Input::Event& event) override;

    // Optional dismiss dispatch when user clicks completely off the menu
    void setOnDismiss(std::function<void()> callback) { onDismiss_ = std::move(callback); }

private:
    std::vector<MenuItem> items_;
    float itemHeight_ = 28.0f;
    int hoveredIndex_ = -1;
    
    std::function<void()> onDismiss_;

    float calculateMenuWidth();
    float calculateMenuHeight();
};

} // namespace Widgets
} // namespace NXRender
