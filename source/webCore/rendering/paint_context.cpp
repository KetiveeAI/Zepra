/**
 * @file paint_context.cpp
 * @brief Display list builder and paint context implementation
 */

#include "rendering/paint_context.hpp"
#include <cstring>

namespace Zepra::WebCore {

// ============================================================================
// DisplayList
// ============================================================================

void DisplayList::addCommand(const PaintCommand& cmd) {
    commands_.push_back(cmd);
}

// ============================================================================
// PaintContext
// ============================================================================

PaintContext::PaintContext(DisplayList& displayList)
    : displayList_(displayList) {
    transform_[0] = 1; transform_[1] = 0;
    transform_[2] = 0; transform_[3] = 1;
    transform_[4] = 0; transform_[5] = 0;
}

void PaintContext::fillRect(const Rect& rect, const Color& color) {
    PaintCommand cmd;
    cmd.type = PaintCommandType::FillRect;
    cmd.rect = rect;
    cmd.color = color;
    memcpy(cmd.matrix, transform_, sizeof(transform_));
    displayList_.addCommand(cmd);
}

void PaintContext::fillRoundedRect(const Rect& rect, const Color& color, float radius) {
    PaintCommand cmd;
    cmd.type = PaintCommandType::FillRoundedRect;
    cmd.rect = rect;
    cmd.color = color;
    cmd.radius = radius;
    memcpy(cmd.matrix, transform_, sizeof(transform_));
    displayList_.addCommand(cmd);
}

void PaintContext::strokeRect(const Rect& rect, const Color& color, float width) {
    PaintCommand cmd;
    cmd.type = PaintCommandType::StrokeRect;
    cmd.rect = rect;
    cmd.color = color;
    cmd.number = width;
    memcpy(cmd.matrix, transform_, sizeof(transform_));
    displayList_.addCommand(cmd);
}

void PaintContext::drawText(const std::string& text, float x, float y,
                            const Color& color, float fontSize) {
    PaintCommand cmd;
    cmd.type = PaintCommandType::DrawText;
    cmd.text = text;
    cmd.rect = {x, y, 0, 0};
    cmd.color = color;
    cmd.number = fontSize;
    memcpy(cmd.matrix, transform_, sizeof(transform_));
    displayList_.addCommand(cmd);
}

void PaintContext::drawImage(const std::string& path, const Rect& dest) {
    PaintCommand cmd;
    cmd.type = PaintCommandType::DrawImage;
    cmd.imagePath = path;
    cmd.rect = dest;
    memcpy(cmd.matrix, transform_, sizeof(transform_));
    displayList_.addCommand(cmd);
}

void PaintContext::drawTexture(uint32_t textureId, const Rect& dest) {
    PaintCommand cmd;
    cmd.type = PaintCommandType::DrawTexture;
    cmd.textureId = textureId;
    cmd.rect = dest;
    memcpy(cmd.matrix, transform_, sizeof(transform_));
    displayList_.addCommand(cmd);
}

void PaintContext::pushClip(const Rect& rect) {
    clipStack_.push_back(rect);
    PaintCommand cmd;
    cmd.type = PaintCommandType::PushClip;
    cmd.rect = rect;
    displayList_.addCommand(cmd);
}

void PaintContext::popClip() {
    if (!clipStack_.empty()) clipStack_.pop_back();
    PaintCommand cmd;
    cmd.type = PaintCommandType::PopClip;
    displayList_.addCommand(cmd);
}

void PaintContext::translate(float x, float y) {
    transform_[4] += x;
    transform_[5] += y;
}

void PaintContext::scale(float sx, float sy) {
    transform_[0] *= sx;
    transform_[3] *= sy;
}

void PaintContext::rotate(float angle) {
    // Not implemented for now — 2D browser doesn't need rotation
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
    // Push clip stack state (transform is mutable)
}

void PaintContext::restore() {
    // Restore is handled by push/popClip pairs
}

} // namespace Zepra::WebCore
