#pragma once

#include "paint_context.hpp"
#include <string>

namespace Zepra::WebCore {

class SimpleFont {
public:
    static void drawText(PaintContext& ctx, const std::string& text, float x, float y, const Color& color, float size);
    static float getTextWidth(const std::string& text, float size);
    static float getCharWidth(float size);
    static float getLineHeight(float size);
};

} // namespace Zepra::WebCore
