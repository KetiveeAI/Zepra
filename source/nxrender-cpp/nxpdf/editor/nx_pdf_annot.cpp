// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "nx_pdf.h"
#include "../renderer/nx_pdf_graphics.h"
#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <algorithm>

namespace nxrender {
namespace pdf {
namespace editor {

// ==================================================================
// Annotation types
// ==================================================================

enum class AnnotationType {
    Text,
    Link,
    FreeText,
    Line,
    Square,
    Circle,
    Polygon,
    Polyline,
    Highlight,
    Underline,
    Strikeout,
    StampAnnot,
    Ink,
    Popup,
    FileAttachment,
    Widget
};

struct AnnotationRect {
    double x1 = 0, y1 = 0, x2 = 0, y2 = 0;

    double width() const { return x2 - x1; }
    double height() const { return y2 - y1; }
    bool contains(double x, double y) const {
        return x >= x1 && x <= x2 && y >= y1 && y <= y2;
    }
};

// ==================================================================
// Annotation
// ==================================================================

class Annotation {
public:
    Annotation() = default;
    explicit Annotation(AnnotationType type) : type_(type) {}

    AnnotationType type() const { return type_; }
    const AnnotationRect& rect() const { return rect_; }
    void setRect(double x1, double y1, double x2, double y2) {
        rect_ = {x1, y1, x2, y2};
    }

    const std::string& contents() const { return contents_; }
    void setContents(const std::string& text) { contents_ = text; }

    const std::string& author() const { return author_; }
    void setAuthor(const std::string& name) { author_ = name; }

    // Color
    void setColor(double r, double g, double b) {
        colorR_ = r; colorG_ = g; colorB_ = b;
    }

    // Border
    void setBorderWidth(double w) { borderWidth_ = w; }
    double borderWidth() const { return borderWidth_; }

    // Popup
    bool hasPopup() const { return hasPopup_; }
    void setHasPopup(bool p) { hasPopup_ = p; }

    // Flags
    bool isHidden() const { return hidden_; }
    void setHidden(bool h) { hidden_ = h; }
    bool isPrintable() const { return printable_; }
    void setPrintable(bool p) { printable_ = p; }

    // Hit testing
    bool hitTest(double x, double y) const {
        return rect_.contains(x, y) && !hidden_;
    }

    // Link destination
    const std::string& linkDest() const { return linkDest_; }
    void setLinkDest(const std::string& dest) { linkDest_ = dest; }
    int linkPage() const { return linkPage_; }
    void setLinkPage(int page) { linkPage_ = page; }

    // Highlight quad points (for text markup annotations)
    void addQuadPoint(double x1, double y1, double x2, double y2,
                      double x3, double y3, double x4, double y4) {
        quadPoints_.push_back({x1, y1, x2, y2, x3, y3, x4, y4});
    }

    // Ink strokes (for ink annotations)
    void beginStroke() { inkStrokes_.emplace_back(); }
    void addStrokePoint(double x, double y) {
        if (!inkStrokes_.empty()) {
            inkStrokes_.back().push_back({x, y});
        }
    }

    uint32_t objectId() const { return objectId_; }
    void setObjectId(uint32_t id) { objectId_ = id; }

private:
    AnnotationType type_ = AnnotationType::Text;
    AnnotationRect rect_;
    std::string contents_;
    std::string author_;
    double colorR_ = 1, colorG_ = 1, colorB_ = 0;
    double borderWidth_ = 1.0;
    bool hasPopup_ = false;
    bool hidden_ = false;
    bool printable_ = true;

    std::string linkDest_;
    int linkPage_ = -1;

    struct QuadPoint {
        double x1, y1, x2, y2, x3, y3, x4, y4;
    };
    std::vector<QuadPoint> quadPoints_;

    struct StrokePoint { double x, y; };
    std::vector<std::vector<StrokePoint>> inkStrokes_;

    uint32_t objectId_ = 0;
};

// ==================================================================
// Annotation manager per page
// ==================================================================

class AnnotationManager {
public:
    AnnotationManager() = default;

    void addAnnotation(std::unique_ptr<Annotation> annot) {
        annotations_.push_back(std::move(annot));
    }

    Annotation* annotationAtPoint(double x, double y) const {
        // Reverse order — top annotation wins
        for (int i = static_cast<int>(annotations_.size()) - 1; i >= 0; i--) {
            if (annotations_[i]->hitTest(x, y)) {
                return annotations_[i].get();
            }
        }
        return nullptr;
    }

    void removeAnnotation(uint32_t objectId) {
        annotations_.erase(
            std::remove_if(annotations_.begin(), annotations_.end(),
                [objectId](const std::unique_ptr<Annotation>& a) {
                    return a->objectId() == objectId;
                }),
            annotations_.end()
        );
    }

    size_t count() const { return annotations_.size(); }
    Annotation* at(size_t index) const {
        return (index < annotations_.size()) ? annotations_[index].get() : nullptr;
    }

    void clear() { annotations_.clear(); }

private:
    std::vector<std::unique_ptr<Annotation>> annotations_;
};

} // namespace editor
} // namespace pdf
} // namespace nxrender
