// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file primitives.h
 * @brief Geometric primitives for NXRender
 */

#pragma once

#include <cmath>
#include <algorithm>

namespace NXRender {

/**
 * @brief 2D Point
 */
struct Point {
    float x = 0;
    float y = 0;
    
    constexpr Point() = default;
    constexpr Point(float x, float y) : x(x), y(y) {}
    
    constexpr Point operator+(const Point& other) const {
        return Point(x + other.x, y + other.y);
    }
    
    constexpr Point operator-(const Point& other) const {
        return Point(x - other.x, y - other.y);
    }
    
    constexpr Point operator*(float scalar) const {
        return Point(x * scalar, y * scalar);
    }
    
    float distance(const Point& other) const {
        float dx = x - other.x;
        float dy = y - other.y;
        return std::sqrt(dx * dx + dy * dy);
    }
    
    static constexpr Point zero() { return Point(0, 0); }
};

/**
 * @brief 2D Size
 */
struct Size {
    float width = 0;
    float height = 0;
    
    constexpr Size() = default;
    constexpr Size(float w, float h) : width(w), height(h) {}
    
    constexpr float area() const { return width * height; }
    constexpr bool isEmpty() const { return width <= 0 || height <= 0; }
    
    static constexpr Size zero() { return Size(0, 0); }
};

/**
 * @brief Rectangle (position + size)
 */
struct Rect {
    float x = 0;
    float y = 0;
    float width = 0;
    float height = 0;
    
    constexpr Rect() = default;
    constexpr Rect(float x, float y, float w, float h)
        : x(x), y(y), width(w), height(h) {}
    constexpr Rect(Point origin, Size size)
        : x(origin.x), y(origin.y), width(size.width), height(size.height) {}
    
    // Accessors
    constexpr Point origin() const { return Point(x, y); }
    constexpr Size size() const { return Size(width, height); }
    constexpr float left() const { return x; }
    constexpr float top() const { return y; }
    constexpr float right() const { return x + width; }
    constexpr float bottom() const { return y + height; }
    constexpr Point center() const { return Point(x + width / 2, y + height / 2); }
    
    // Hit testing
    constexpr bool contains(Point p) const {
        return p.x >= x && p.x < x + width && p.y >= y && p.y < y + height;
    }
    
    constexpr bool contains(float px, float py) const {
        return px >= x && px < x + width && py >= y && py < y + height;
    }
    
    // Intersection
    constexpr bool intersects(const Rect& other) const {
        return x < other.right() && right() > other.x &&
               y < other.bottom() && bottom() > other.y;
    }
    
    Rect intersection(const Rect& other) const {
        float newX = std::max(x, other.x);
        float newY = std::max(y, other.y);
        float newRight = std::min(right(), other.right());
        float newBottom = std::min(bottom(), other.bottom());
        if (newRight <= newX || newBottom <= newY) {
            return Rect();
        }
        return Rect(newX, newY, newRight - newX, newBottom - newY);
    }
    
    // Union (bounding box)
    Rect unite(const Rect& other) const {
        if (isEmpty()) return other;
        if (other.isEmpty()) return *this;
        float newX = std::min(x, other.x);
        float newY = std::min(y, other.y);
        float newRight = std::max(right(), other.right());
        float newBottom = std::max(bottom(), other.bottom());
        return Rect(newX, newY, newRight - newX, newBottom - newY);
    }
    
    // Inset (shrink by amount on each side)
    constexpr Rect inset(float amount) const {
        return Rect(x + amount, y + amount, width - amount * 2, height - amount * 2);
    }
    
    constexpr Rect inset(float h, float v) const {
        return Rect(x + h, y + v, width - h * 2, height - v * 2);
    }
    
    // Offset
    constexpr Rect offset(float dx, float dy) const {
        return Rect(x + dx, y + dy, width, height);
    }
    
    constexpr bool isEmpty() const { return width <= 0 || height <= 0; }
    
    static constexpr Rect zero() { return Rect(0, 0, 0, 0); }
};

/**
 * @brief Edge insets (margin/padding)
 */
struct EdgeInsets {
    float top = 0;
    float right = 0;
    float bottom = 0;
    float left = 0;
    
    constexpr EdgeInsets() = default;
    constexpr EdgeInsets(float all) : top(all), right(all), bottom(all), left(all) {}
    constexpr EdgeInsets(float v, float h) : top(v), right(h), bottom(v), left(h) {}
    constexpr EdgeInsets(float t, float r, float b, float l)
        : top(t), right(r), bottom(b), left(l) {}
    
    constexpr float horizontal() const { return left + right; }
    constexpr float vertical() const { return top + bottom; }
    
    static constexpr EdgeInsets zero() { return EdgeInsets(0); }
};

/**
 * @brief Corner radii
 */
struct CornerRadii {
    float topLeft = 0;
    float topRight = 0;
    float bottomRight = 0;
    float bottomLeft = 0;
    
    constexpr CornerRadii() = default;
    constexpr CornerRadii(float all)
        : topLeft(all), topRight(all), bottomRight(all), bottomLeft(all) {}
    constexpr CornerRadii(float tl, float tr, float br, float bl)
        : topLeft(tl), topRight(tr), bottomRight(br), bottomLeft(bl) {}
    
    constexpr bool isUniform() const {
        return topLeft == topRight && topRight == bottomRight && bottomRight == bottomLeft;
    }
    
    static constexpr CornerRadii zero() { return CornerRadii(0); }
};

} // namespace NXRender
