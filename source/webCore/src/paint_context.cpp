/**
 * @file paint_context.cpp
 * @brief Paint context implementation
 */

#include "webcore/paint_context.hpp"

namespace Zepra::WebCore {

// =============================================================================
// DisplayList
// =============================================================================

void DisplayList::addCommand(const PaintCommand& cmd) {
    commands_.push_back(cmd);
}

// =============================================================================
// PaintContext
// =============================================================================

PaintContext::PaintContext(DisplayList& displayList) : displayList_(displayList) {}

void PaintContext::fillRect(const Rect& rect, const Color& color) {
    PaintCommand cmd;
    cmd.type = PaintCommandType::FillRect;
    cmd.rect = rect;
    cmd.color = color;
    displayList_.addCommand(cmd);
}

void PaintContext::strokeRect(const Rect& rect, const Color& color, float width) {
    // Draw 4 lines for stroke
    PaintCommand cmd;
    cmd.type = PaintCommandType::StrokeRect;
    cmd.rect = rect;
    cmd.color = color;
    cmd.number = width;
    displayList_.addCommand(cmd);
}

void PaintContext::drawText(const std::string& text, float x, float y, 
                            const Color& color, float fontSize) {
    PaintCommand cmd;
    cmd.type = PaintCommandType::DrawText;
    cmd.rect = {x, y, 0, fontSize};
    cmd.color = color;
    cmd.text = text;
    displayList_.addCommand(cmd);
}

void PaintContext::drawImage(const std::string& path, const Rect& dest) {
    PaintCommand cmd;
    cmd.type = PaintCommandType::DrawImage;
    cmd.rect = dest;
    cmd.imagePath = path;
    displayList_.addCommand(cmd);
}

void PaintContext::drawTexture(uint32_t textureId, const Rect& dest) {
    PaintCommand cmd;
    cmd.type = PaintCommandType::DrawTexture;
    cmd.rect = dest;
    cmd.textureId = textureId;
    displayList_.addCommand(cmd);
}

void PaintContext::pushClip(const Rect& rect) {
    PaintCommand cmd;
    cmd.type = PaintCommandType::PushClip;
    cmd.rect = rect;
    displayList_.addCommand(cmd);
    
    if (clipStack_.empty()) {
        clipStack_.push_back(rect);
    } else {
        clipStack_.push_back(clipStack_.back().intersected(rect));
    }
}

void PaintContext::popClip() {
    PaintCommand cmd;
    cmd.type = PaintCommandType::PopClip;
    displayList_.addCommand(cmd);
    
    if (!clipStack_.empty()) {
        clipStack_.pop_back();
    }
}

void PaintContext::translate(float x, float y) {
    transform_[4] += x;
    transform_[5] += y;
    
    PaintCommand cmd;
    cmd.type = PaintCommandType::SetTransform;
    std::copy(std::begin(transform_), std::end(transform_), std::begin(cmd.matrix));
    displayList_.addCommand(cmd);
}

void PaintContext::scale(float sx, float sy) {
    transform_[0] *= sx;
    transform_[3] *= sy;
    
    PaintCommand cmd;
    cmd.type = PaintCommandType::SetTransform;
    std::copy(std::begin(transform_), std::end(transform_), std::begin(cmd.matrix));
    displayList_.addCommand(cmd);
}

void PaintContext::rotate(float) {
    // TODO: Implement rotation
}

void PaintContext::resetTransform() {
    transform_[0] = 1; transform_[1] = 0;
    transform_[2] = 0; transform_[3] = 1;
    transform_[4] = 0; transform_[5] = 0;
    
    PaintCommand cmd;
    cmd.type = PaintCommandType::ResetTransform;
    displayList_.addCommand(cmd);
}

void PaintContext::save() {
    // TODO: Save state stack
}

void PaintContext::restore() {
    // TODO: Restore state stack
}

} // namespace Zepra::WebCore
