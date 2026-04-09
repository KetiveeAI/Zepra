// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "widgets/dropdown.h"
#include "nxgfx/context.h"
#include <algorithm>
#include <cmath>

namespace NXRender {

// ==================================================================
// Dropdown
// ==================================================================

Dropdown::Dropdown() {
    backgroundColor_ = Color(255, 255, 255);
}

Dropdown::Dropdown(const std::vector<DropdownOption>& options) : options_(options) {
    backgroundColor_ = Color(255, 255, 255);
}

void Dropdown::setOptions(const std::vector<DropdownOption>& options) {
    options_ = options;
    selectedIndex_ = -1;
    scrollOffset_ = 0;
    invalidate();
}

void Dropdown::addOption(const DropdownOption& option) {
    options_.push_back(option);
    invalidate();
}

void Dropdown::removeOption(int index) {
    if (index < 0 || index >= static_cast<int>(options_.size())) return;
    options_.erase(options_.begin() + index);
    if (selectedIndex_ == index) selectedIndex_ = -1;
    else if (selectedIndex_ > index) selectedIndex_--;
    invalidate();
}

void Dropdown::clearOptions() {
    options_.clear();
    selectedIndex_ = -1;
    scrollOffset_ = 0;
    invalidate();
}

void Dropdown::setSelectedIndex(int index) {
    if (index < -1 || index >= static_cast<int>(options_.size())) return;
    if (index != selectedIndex_) {
        selectedIndex_ = index;
        if (onSelect_ && index >= 0) {
            onSelect_(index, options_[static_cast<size_t>(index)]);
        }
        invalidate();
    }
}

const DropdownOption* Dropdown::selectedOption() const {
    if (selectedIndex_ < 0 || selectedIndex_ >= static_cast<int>(options_.size())) return nullptr;
    return &options_[static_cast<size_t>(selectedIndex_)];
}

std::string Dropdown::selectedValue() const {
    const DropdownOption* opt = selectedOption();
    return opt ? opt->value : "";
}

void Dropdown::selectByValue(const std::string& value) {
    for (int i = 0; i < static_cast<int>(options_.size()); i++) {
        if (options_[static_cast<size_t>(i)].value == value) {
            setSelectedIndex(i);
            return;
        }
    }
}

void Dropdown::open() {
    isOpen_ = true;
    hoveredIndex_ = selectedIndex_;
    searchText_.clear();
    invalidate();
}

void Dropdown::close() {
    isOpen_ = false;
    hoveredIndex_ = -1;
    searchText_.clear();
    invalidate();
}

void Dropdown::toggle() {
    if (isOpen_) close();
    else open();
}

std::vector<int> Dropdown::filteredIndices() const {
    std::vector<int> indices;
    for (int i = 0; i < static_cast<int>(options_.size()); i++) {
        const auto& opt = options_[static_cast<size_t>(i)];
        if (opt.separator) {
            indices.push_back(i);
            continue;
        }
        if (searchText_.empty()) {
            indices.push_back(i);
            continue;
        }
        // Case-insensitive search
        std::string labelLower = opt.label;
        std::string searchLower = searchText_;
        std::transform(labelLower.begin(), labelLower.end(), labelLower.begin(), ::tolower);
        std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(), ::tolower);
        if (labelLower.find(searchLower) != std::string::npos) {
            indices.push_back(i);
        }
    }
    return indices;
}

Rect Dropdown::dropdownRect() const {
    const Rect& b = bounds();
    auto indices = filteredIndices();
    float contentHeight = 0;
    for (int idx : indices) {
        const auto& opt = options_[static_cast<size_t>(idx)];
        contentHeight += opt.separator ? 9.0f : itemHeight_;
    }
    float height = std::min(contentHeight, maxDropdownHeight_);
    return Rect(b.x, b.y + b.height + 2, b.width, height);
}

Size Dropdown::preferredSize() const {
    float maxWidth = 120.0f;
    for (const auto& opt : options_) {
        float w = static_cast<float>(opt.label.size()) * 8.0f + 40;
        maxWidth = std::max(maxWidth, w);
    }
    return Size(maxWidth, 36.0f);
}

bool Dropdown::handleEvent(const Event& event) {
    if (!isEnabled()) return false;

    if (event.type == EventType::MouseDown) {
        const Rect& b = bounds();

        // Click on button area
        if (b.contains(event.mouseX, event.mouseY)) {
            toggle();
            return true;
        }

        // Click in dropdown list
        if (isOpen_) {
            Rect dr = dropdownRect();
            if (dr.contains(event.mouseX, event.mouseY)) {
                auto indices = filteredIndices();
                float y = dr.y - scrollOffset_;
                for (int idx : indices) {
                    const auto& opt = options_[static_cast<size_t>(idx)];
                    float h = opt.separator ? 9.0f : itemHeight_;
                    if (event.mouseY >= y && event.mouseY < y + h) {
                        if (!opt.separator && !opt.disabled) {
                            setSelectedIndex(idx);
                            close();
                        }
                        return true;
                    }
                    y += h;
                }
            }

            // Click outside — close
            close();
            return true;
        }
    }

    if (event.type == EventType::MouseMove && isOpen_) {
        Rect dr = dropdownRect();
        if (dr.contains(event.mouseX, event.mouseY)) {
            auto indices = filteredIndices();
            float y = dr.y - scrollOffset_;
            for (int idx : indices) {
                const auto& opt = options_[static_cast<size_t>(idx)];
                float h = opt.separator ? 9.0f : itemHeight_;
                if (event.mouseY >= y && event.mouseY < y + h) {
                    if (hoveredIndex_ != idx) {
                        hoveredIndex_ = idx;
                        invalidate();
                    }
                    return true;
                }
                y += h;
            }
        }
    }

    if (event.type == EventType::MouseWheel && isOpen_) {
        Rect dr = dropdownRect();
        if (dr.contains(event.mouseX, event.mouseY)) {
            scrollOffset_ -= event.scrollDelta * 30.0f;
            scrollOffset_ = std::max(0.0f, scrollOffset_);
            invalidate();
            return true;
        }
    }

    if (event.type == EventType::KeyDown && isOpen_) {
        if (event.keyCode == KeyCode::Escape) {
            close();
            return true;
        }
        if (event.keyCode == KeyCode::Enter) {
            if (hoveredIndex_ >= 0 && hoveredIndex_ < static_cast<int>(options_.size())) {
                setSelectedIndex(hoveredIndex_);
                close();
            }
            return true;
        }
        if (event.keyCode == KeyCode::Up) {
            auto indices = filteredIndices();
            for (int i = static_cast<int>(indices.size()) - 1; i >= 0; i--) {
                if (indices[static_cast<size_t>(i)] < hoveredIndex_ &&
                    !options_[static_cast<size_t>(indices[static_cast<size_t>(i)])].separator &&
                    !options_[static_cast<size_t>(indices[static_cast<size_t>(i)])].disabled) {
                    hoveredIndex_ = indices[static_cast<size_t>(i)];
                    invalidate();
                    break;
                }
            }
            return true;
        }
        if (event.keyCode == KeyCode::Down) {
            auto indices = filteredIndices();
            for (size_t i = 0; i < indices.size(); i++) {
                if (indices[i] > hoveredIndex_ &&
                    !options_[static_cast<size_t>(indices[i])].separator &&
                    !options_[static_cast<size_t>(indices[i])].disabled) {
                    hoveredIndex_ = indices[i];
                    invalidate();
                    break;
                }
            }
            return true;
        }

        // Type-to-search
        if (searchable_ && event.keyCode >= KeyCode::A && event.keyCode <= KeyCode::Z) {
            char c = 'a' + (static_cast<int>(event.keyCode) - static_cast<int>(KeyCode::A));
            searchText_ += c;
            invalidate();
            return true;
        }
        if (searchable_ && event.keyCode == KeyCode::Backspace && !searchText_.empty()) {
            searchText_.pop_back();
            invalidate();
            return true;
        }
    }

    return false;
}

void Dropdown::renderButton(GpuContext* gpu) {
    const Rect& b = bounds();

    // Button background
    Color bg = isOpen_ ? Color(0xE8EAF6) : backgroundColor_;
    gpu->fillRoundedRect(b, bg, 4.0f);
    gpu->strokeRoundedRect(b, Color(0xBDBDBD), 4.0f, 1.0f);

    // Selected text or placeholder
    std::string displayText = placeholder_;
    Color textColor = Color(0x9E9E9E);
    if (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(options_.size())) {
        displayText = options_[static_cast<size_t>(selectedIndex_)].label;
        textColor = Color(0x212121);
    }
    gpu->drawText(displayText, b.x + 12, b.y + (b.height - 14) * 0.5f, textColor, 14.0f);

    // Dropdown arrow
    float arrowX = b.x + b.width - 24;
    float arrowCY = b.y + b.height * 0.5f;
    float arrowSize = 4.0f;
    if (isOpen_) {
        gpu->drawLine(arrowX, arrowCY + arrowSize * 0.5f,
                      arrowX + arrowSize, arrowCY - arrowSize * 0.5f,
                      Color(0x757575), 1.5f);
        gpu->drawLine(arrowX + arrowSize, arrowCY - arrowSize * 0.5f,
                      arrowX + arrowSize * 2, arrowCY + arrowSize * 0.5f,
                      Color(0x757575), 1.5f);
    } else {
        gpu->drawLine(arrowX, arrowCY - arrowSize * 0.5f,
                      arrowX + arrowSize, arrowCY + arrowSize * 0.5f,
                      Color(0x757575), 1.5f);
        gpu->drawLine(arrowX + arrowSize, arrowCY + arrowSize * 0.5f,
                      arrowX + arrowSize * 2, arrowCY - arrowSize * 0.5f,
                      Color(0x757575), 1.5f);
    }
}

void Dropdown::renderOption(GpuContext* gpu, const DropdownOption& opt, int index, float y) {
    const Rect& b = bounds();

    if (opt.separator) {
        gpu->drawLine(b.x + 8, y + 4.5f, b.x + b.width - 8, y + 4.5f,
                      Color(0xE0E0E0), 1.0f);
        return;
    }

    Rect itemRect(b.x, y, b.width, itemHeight_);

    // Background on hover/selected
    if (index == hoveredIndex_) {
        gpu->fillRect(itemRect, hoverBg_);
    } else if (index == selectedIndex_) {
        gpu->fillRect(itemRect, Color(0xE3F2FD));
    }

    // Text
    Color textColor = opt.disabled ? Color(0xBDBDBD) : Color(0x212121);
    gpu->drawText(opt.label, b.x + 12, y + (itemHeight_ - 14) * 0.5f, textColor, 14.0f);

    // Checkmark for selected item
    if (index == selectedIndex_) {
        float checkX = b.x + b.width - 24;
        float checkY = y + itemHeight_ * 0.5f;
        gpu->drawLine(checkX, checkY, checkX + 4, checkY + 4, Color(0x2196F3), 2.0f);
        gpu->drawLine(checkX + 4, checkY + 4, checkX + 10, checkY - 4, Color(0x2196F3), 2.0f);
    }
}

void Dropdown::renderDropdown(GpuContext* gpu) {
    Rect dr = dropdownRect();

    // Shadow
    gpu->drawShadow(dr, Color(0, 0, 0, 60), 12.0f, 0, 2);

    // Background
    gpu->fillRoundedRect(dr, dropdownBg_, 4.0f);
    gpu->strokeRoundedRect(dr, Color(0xE0E0E0), 4.0f, 1.0f);

    // Search box at top
    if (searchable_ && !searchText_.empty()) {
        Rect searchRect(dr.x + 8, dr.y + 4, dr.width - 16, 28);
        gpu->fillRoundedRect(searchRect, Color(0xF5F5F5), 4.0f);
        gpu->drawText(searchText_, searchRect.x + 8, searchRect.y + 6, Color(0x212121), 13.0f);
    }

    // Clip to dropdown bounds
    gpu->pushClip(dr);

    auto indices = filteredIndices();
    float y = dr.y - scrollOffset_;

    for (int idx : indices) {
        const auto& opt = options_[static_cast<size_t>(idx)];
        float h = opt.separator ? 9.0f : itemHeight_;

        // Only render visible items
        if (y + h > dr.y && y < dr.y + dr.height) {
            renderOption(gpu, opt, idx, y);
        }
        y += h;
    }

    gpu->popClip();
}

void Dropdown::render(GpuContext* gpu) {
    if (!gpu || !isVisible()) return;

    renderButton(gpu);

    if (isOpen_) {
        renderDropdown(gpu);
    }
}

} // namespace NXRender
