// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file nxrender_tests.cpp
 * @brief Comprehensive GTest suite for NXRender-cpp
 */

#include <gtest/gtest.h>
#include "nxgfx/color.h"
#include "nxgfx/primitives.h"
#include "core/compositor.h"
#include "widgets/widget.h"
#include "widgets/container.h"
#include "widgets/label.h"
#include "widgets/button.h"
#include "layout/flexbox.h"
#include "input/events.h"
#include "animation/animator.h"
#include "animation/easing.h"
#include "theme/theme.h"

using namespace NXRender;

// =========================================================================
// Color Tests
// =========================================================================

TEST(Color, DefaultConstruction) {
    Color c;
    EXPECT_EQ(c.r, 0);
    EXPECT_EQ(c.g, 0);
    EXPECT_EQ(c.b, 0);
    EXPECT_EQ(c.a, 255);
}

TEST(Color, RGBConstruction) {
    Color c(128, 64, 32);
    EXPECT_EQ(c.r, 128);
    EXPECT_EQ(c.g, 64);
    EXPECT_EQ(c.b, 32);
    EXPECT_EQ(c.a, 255);
}

TEST(Color, RGBAConstruction) {
    Color c(10, 20, 30, 40);
    EXPECT_EQ(c.r, 10);
    EXPECT_EQ(c.g, 20);
    EXPECT_EQ(c.b, 30);
    EXPECT_EQ(c.a, 40);
}

TEST(Color, HexConstruction) {
    Color c(0xFF8000);
    EXPECT_EQ(c.r, 255);
    EXPECT_EQ(c.g, 128);
    EXPECT_EQ(c.b, 0);
    EXPECT_EQ(c.a, 255);
}

TEST(Color, HexWithAlpha) {
    Color c(0x80FF8000, true);
    EXPECT_EQ(c.a, 128);
    EXPECT_EQ(c.r, 255);
    EXPECT_EQ(c.g, 128);
    EXPECT_EQ(c.b, 0);
}

TEST(Color, ToARGB) {
    Color c(255, 128, 0, 200);
    uint32_t argb = c.toARGB();
    EXPECT_EQ(argb, 0xC8FF8000u);
}

TEST(Color, ToRGBA) {
    Color c(255, 128, 0, 200);
    uint32_t rgba = c.toRGBA();
    EXPECT_EQ(rgba, 0xFF8000C8u);
}

TEST(Color, Equality) {
    EXPECT_EQ(Color::red(), Color(255, 0, 0));
    EXPECT_NE(Color::red(), Color::blue());
}

TEST(Color, Blend) {
    Color a = Color::black();
    Color b = Color::white();
    Color mid = a.blend(b, 0.5f);
    EXPECT_NEAR(mid.r, 127, 1);
    EXPECT_NEAR(mid.g, 127, 1);
    EXPECT_NEAR(mid.b, 127, 1);
}

TEST(Color, Darken) {
    Color c = Color::white();
    Color dark = c.darken(0.5f);
    EXPECT_NEAR(dark.r, 127, 1);
    EXPECT_NEAR(dark.g, 127, 1);
    EXPECT_NEAR(dark.b, 127, 1);
    EXPECT_EQ(dark.a, 255);
}

TEST(Color, Lighten) {
    Color c = Color::black();
    Color light = c.lighten(0.5f);
    EXPECT_NEAR(light.r, 127, 1);
    EXPECT_NEAR(light.g, 127, 1);
    EXPECT_NEAR(light.b, 127, 1);
}

TEST(Color, WithAlpha) {
    Color c = Color::red();
    Color semi = c.withAlpha(128);
    EXPECT_EQ(semi.r, 255);
    EXPECT_EQ(semi.g, 0);
    EXPECT_EQ(semi.b, 0);
    EXPECT_EQ(semi.a, 128);
}

TEST(Color, IsTransparentOpaque) {
    EXPECT_TRUE(Color::transparent().isTransparent());
    EXPECT_FALSE(Color::transparent().isOpaque());
    EXPECT_TRUE(Color::red().isOpaque());
    EXPECT_FALSE(Color::red().isTransparent());
}

TEST(Color, NamedColors) {
    EXPECT_EQ(Color::black(), Color(0, 0, 0));
    EXPECT_EQ(Color::white(), Color(255, 255, 255));
    EXPECT_EQ(Color::red(), Color(255, 0, 0));
    EXPECT_EQ(Color::green(), Color(0, 255, 0));
    EXPECT_EQ(Color::blue(), Color(0, 0, 255));
    EXPECT_EQ(Color::transparent(), Color(0, 0, 0, 0));
}

// =========================================================================
// Color HSL Tests
// =========================================================================

TEST(ColorHSL, RedToHSL) {
    HSL hsl = Color::red().toHSL();
    EXPECT_NEAR(hsl.h, 0.0f, 1.0f);
    EXPECT_NEAR(hsl.s, 1.0f, 0.01f);
    EXPECT_NEAR(hsl.l, 0.5f, 0.01f);
}

TEST(ColorHSL, GreenToHSL) {
    Color green(0, 255, 0);
    HSL hsl = green.toHSL();
    EXPECT_NEAR(hsl.h, 120.0f, 1.0f);
    EXPECT_NEAR(hsl.s, 1.0f, 0.01f);
    EXPECT_NEAR(hsl.l, 0.5f, 0.01f);
}

TEST(ColorHSL, BlueToHSL) {
    HSL hsl = Color::blue().toHSL();
    EXPECT_NEAR(hsl.h, 240.0f, 1.0f);
    EXPECT_NEAR(hsl.s, 1.0f, 0.01f);
    EXPECT_NEAR(hsl.l, 0.5f, 0.01f);
}

TEST(ColorHSL, GrayToHSL) {
    Color gray(128, 128, 128);
    HSL hsl = gray.toHSL();
    EXPECT_NEAR(hsl.s, 0.0f, 0.01f);
    EXPECT_NEAR(hsl.l, 0.5f, 0.02f);
}

TEST(ColorHSL, FromHSLRed) {
    Color c = Color::fromHSL(0, 1.0f, 0.5f);
    EXPECT_EQ(c.r, 255);
    EXPECT_NEAR(c.g, 0, 1);
    EXPECT_NEAR(c.b, 0, 1);
}

TEST(ColorHSL, FromHSLGreen) {
    Color c = Color::fromHSL(120, 1.0f, 0.5f);
    EXPECT_NEAR(c.r, 0, 1);
    EXPECT_EQ(c.g, 255);
    EXPECT_NEAR(c.b, 0, 1);
}

TEST(ColorHSL, FromHSLBlue) {
    Color c = Color::fromHSL(240, 1.0f, 0.5f);
    EXPECT_NEAR(c.r, 0, 1);
    EXPECT_NEAR(c.g, 0, 1);
    EXPECT_EQ(c.b, 255);
}

TEST(ColorHSL, Roundtrip) {
    Color orig(180, 90, 45);
    HSL hsl = orig.toHSL();
    Color back = Color::fromHSL(hsl.h, hsl.s, hsl.l);
    EXPECT_NEAR(back.r, orig.r, 2);
    EXPECT_NEAR(back.g, orig.g, 2);
    EXPECT_NEAR(back.b, orig.b, 2);
}

// =========================================================================
// Color Premultiply Tests
// =========================================================================

TEST(ColorPremultiply, FullAlpha) {
    Color c(200, 100, 50, 255);
    Color pm = c.premultiply();
    EXPECT_EQ(pm.r, 200);
    EXPECT_EQ(pm.g, 100);
    EXPECT_EQ(pm.b, 50);
}

TEST(ColorPremultiply, ZeroAlpha) {
    Color c(200, 100, 50, 0);
    Color pm = c.premultiply();
    EXPECT_EQ(pm.r, 0);
    EXPECT_EQ(pm.g, 0);
    EXPECT_EQ(pm.b, 0);
}

TEST(ColorPremultiply, HalfAlpha) {
    Color c(200, 100, 50, 128);
    Color pm = c.premultiply();
    EXPECT_NEAR(pm.r, 100, 2);
    EXPECT_NEAR(pm.g, 50, 2);
    EXPECT_NEAR(pm.b, 25, 2);
    EXPECT_EQ(pm.a, 128);
}

TEST(ColorPremultiply, UnpremultiplyRoundtrip) {
    Color orig(200, 100, 50, 128);
    Color pm = orig.premultiply();
    Color back = pm.unpremultiply();
    EXPECT_NEAR(back.r, orig.r, 2);
    EXPECT_NEAR(back.g, orig.g, 2);
    EXPECT_NEAR(back.b, orig.b, 2);
}

// =========================================================================
// Color Parse Tests
// =========================================================================

TEST(ColorParse, HexRGB) {
    Color c = Color::parse("#F80");
    EXPECT_EQ(c.r, 255);
    EXPECT_EQ(c.g, 136);
    EXPECT_EQ(c.b, 0);
}

TEST(ColorParse, HexRRGGBB) {
    Color c = Color::parse("#FF8800");
    EXPECT_EQ(c.r, 255);
    EXPECT_EQ(c.g, 136);
    EXPECT_EQ(c.b, 0);
}

TEST(ColorParse, HexRRGGBBAA) {
    Color c = Color::parse("#FF880080");
    EXPECT_EQ(c.r, 255);
    EXPECT_EQ(c.g, 136);
    EXPECT_EQ(c.b, 0);
    EXPECT_EQ(c.a, 128);
}

TEST(ColorParse, RGBFunction) {
    Color c = Color::parse("rgb(100, 200, 50)");
    EXPECT_EQ(c.r, 100);
    EXPECT_EQ(c.g, 200);
    EXPECT_EQ(c.b, 50);
    EXPECT_EQ(c.a, 255);
}

TEST(ColorParse, RGBAFunction) {
    Color c = Color::parse("rgba(100, 200, 50, 0.5)");
    EXPECT_EQ(c.r, 100);
    EXPECT_EQ(c.g, 200);
    EXPECT_EQ(c.b, 50);
    EXPECT_NEAR(c.a, 127, 1);
}

TEST(ColorParse, HSLFunction) {
    Color c = Color::parse("hsl(0, 100%, 50%)");
    EXPECT_EQ(c.r, 255);
    EXPECT_NEAR(c.g, 0, 1);
    EXPECT_NEAR(c.b, 0, 1);
}

TEST(ColorParse, HSLAFunction) {
    Color c = Color::parse("hsla(120, 100%, 50%, 0.5)");
    EXPECT_NEAR(c.r, 0, 1);
    EXPECT_EQ(c.g, 255);
    EXPECT_NEAR(c.b, 0, 1);
    EXPECT_NEAR(c.a, 127, 1);
}

TEST(ColorParse, NamedColorRed) {
    Color c = Color::parse("red");
    EXPECT_EQ(c, Color::red());
}

TEST(ColorParse, NamedColorCoral) {
    Color c = Color::parse("coral");
    EXPECT_EQ(c.r, 255);
    EXPECT_EQ(c.g, 127);
    EXPECT_EQ(c.b, 80);
}

TEST(ColorParse, NamedColorTransparent) {
    Color c = Color::parse("transparent");
    EXPECT_EQ(c, Color::transparent());
}

TEST(ColorParse, EmptyString) {
    Color c = Color::parse("");
    EXPECT_EQ(c, Color::black());
}

TEST(ColorParse, WhitespaceHandling) {
    Color c = Color::parse("  #FF0000  ");
    EXPECT_EQ(c, Color::red());
}

// =========================================================================
// Point Tests
// =========================================================================

TEST(Point, DefaultConstruction) {
    Point p;
    EXPECT_FLOAT_EQ(p.x, 0);
    EXPECT_FLOAT_EQ(p.y, 0);
}

TEST(Point, ValueConstruction) {
    Point p(10, 20);
    EXPECT_FLOAT_EQ(p.x, 10);
    EXPECT_FLOAT_EQ(p.y, 20);
}

TEST(Point, Addition) {
    Point a(1, 2);
    Point b(3, 4);
    Point c = a + b;
    EXPECT_FLOAT_EQ(c.x, 4);
    EXPECT_FLOAT_EQ(c.y, 6);
}

TEST(Point, Subtraction) {
    Point a(5, 7);
    Point b(2, 3);
    Point c = a - b;
    EXPECT_FLOAT_EQ(c.x, 3);
    EXPECT_FLOAT_EQ(c.y, 4);
}

TEST(Point, Multiply) {
    Point p(3, 4);
    Point scaled = p * 2.0f;
    EXPECT_FLOAT_EQ(scaled.x, 6);
    EXPECT_FLOAT_EQ(scaled.y, 8);
}

TEST(Point, Distance) {
    Point a(0, 0);
    Point b(3, 4);
    EXPECT_FLOAT_EQ(a.distance(b), 5.0f);
}

TEST(Point, Zero) {
    Point z = Point::zero();
    EXPECT_FLOAT_EQ(z.x, 0);
    EXPECT_FLOAT_EQ(z.y, 0);
}

// =========================================================================
// Size Tests
// =========================================================================

TEST(Size, DefaultConstruction) {
    Size s;
    EXPECT_FLOAT_EQ(s.width, 0);
    EXPECT_FLOAT_EQ(s.height, 0);
}

TEST(Size, ValueConstruction) {
    Size s(100, 200);
    EXPECT_FLOAT_EQ(s.width, 100);
    EXPECT_FLOAT_EQ(s.height, 200);
}

TEST(Size, Area) {
    Size s(10, 20);
    EXPECT_FLOAT_EQ(s.area(), 200);
}

TEST(Size, IsEmpty) {
    EXPECT_TRUE(Size(0, 0).isEmpty());
    EXPECT_TRUE(Size(-1, 10).isEmpty());
    EXPECT_TRUE(Size(10, 0).isEmpty());
    EXPECT_FALSE(Size(1, 1).isEmpty());
}

// =========================================================================
// Rect Tests
// =========================================================================

TEST(Rect, DefaultConstruction) {
    Rect r;
    EXPECT_FLOAT_EQ(r.x, 0);
    EXPECT_FLOAT_EQ(r.y, 0);
    EXPECT_FLOAT_EQ(r.width, 0);
    EXPECT_FLOAT_EQ(r.height, 0);
}

TEST(Rect, ValueConstruction) {
    Rect r(10, 20, 100, 200);
    EXPECT_FLOAT_EQ(r.x, 10);
    EXPECT_FLOAT_EQ(r.y, 20);
    EXPECT_FLOAT_EQ(r.width, 100);
    EXPECT_FLOAT_EQ(r.height, 200);
}

TEST(Rect, AccessorsMethods) {
    Rect r(10, 20, 100, 200);
    EXPECT_FLOAT_EQ(r.left(), 10);
    EXPECT_FLOAT_EQ(r.top(), 20);
    EXPECT_FLOAT_EQ(r.right(), 110);
    EXPECT_FLOAT_EQ(r.bottom(), 220);
    Point c = r.center();
    EXPECT_FLOAT_EQ(c.x, 60);
    EXPECT_FLOAT_EQ(c.y, 120);
}

TEST(Rect, ContainsPoint) {
    Rect r(10, 10, 100, 100);
    EXPECT_TRUE(r.contains(50, 50));
    EXPECT_TRUE(r.contains(10, 10));
    EXPECT_FALSE(r.contains(110, 50)); // right edge exclusive
    EXPECT_FALSE(r.contains(5, 50));
    EXPECT_FALSE(r.contains(50, 5));
}

TEST(Rect, ContainsPointStruct) {
    Rect r(0, 0, 100, 100);
    EXPECT_TRUE(r.contains(Point(50, 50)));
    EXPECT_FALSE(r.contains(Point(100, 100)));
}

TEST(Rect, Intersects) {
    Rect a(0, 0, 100, 100);
    Rect b(50, 50, 100, 100);
    Rect c(200, 200, 50, 50);
    EXPECT_TRUE(a.intersects(b));
    EXPECT_TRUE(b.intersects(a));
    EXPECT_FALSE(a.intersects(c));
}

TEST(Rect, IntersectsEdgeCase) {
    Rect a(0, 0, 100, 100);
    Rect b(100, 0, 100, 100); // touching edge
    EXPECT_FALSE(a.intersects(b));
}

TEST(Rect, Intersection) {
    Rect a(0, 0, 100, 100);
    Rect b(50, 50, 100, 100);
    Rect i = a.intersection(b);
    EXPECT_FLOAT_EQ(i.x, 50);
    EXPECT_FLOAT_EQ(i.y, 50);
    EXPECT_FLOAT_EQ(i.width, 50);
    EXPECT_FLOAT_EQ(i.height, 50);
}

TEST(Rect, IntersectionNoOverlap) {
    Rect a(0, 0, 50, 50);
    Rect b(100, 100, 50, 50);
    Rect i = a.intersection(b);
    EXPECT_TRUE(i.isEmpty());
}

TEST(Rect, Unite) {
    Rect a(10, 10, 50, 50);
    Rect b(40, 40, 50, 50);
    Rect u = a.unite(b);
    EXPECT_FLOAT_EQ(u.x, 10);
    EXPECT_FLOAT_EQ(u.y, 10);
    EXPECT_FLOAT_EQ(u.width, 80);
    EXPECT_FLOAT_EQ(u.height, 80);
}

TEST(Rect, Inset) {
    Rect r(0, 0, 100, 100);
    Rect inset = r.inset(10);
    EXPECT_FLOAT_EQ(inset.x, 10);
    EXPECT_FLOAT_EQ(inset.y, 10);
    EXPECT_FLOAT_EQ(inset.width, 80);
    EXPECT_FLOAT_EQ(inset.height, 80);
}

TEST(Rect, Offset) {
    Rect r(10, 20, 30, 40);
    Rect off = r.offset(5, -5);
    EXPECT_FLOAT_EQ(off.x, 15);
    EXPECT_FLOAT_EQ(off.y, 15);
    EXPECT_FLOAT_EQ(off.width, 30);
    EXPECT_FLOAT_EQ(off.height, 40);
}

TEST(Rect, IsEmpty) {
    EXPECT_TRUE(Rect::zero().isEmpty());
    EXPECT_TRUE(Rect(0, 0, 0, 100).isEmpty());
    EXPECT_TRUE(Rect(0, 0, -1, 100).isEmpty());
    EXPECT_FALSE(Rect(0, 0, 1, 1).isEmpty());
}

// =========================================================================
// EdgeInsets Tests
// =========================================================================

TEST(EdgeInsets, DefaultConstruction) {
    EdgeInsets e;
    EXPECT_FLOAT_EQ(e.top, 0);
    EXPECT_FLOAT_EQ(e.right, 0);
    EXPECT_FLOAT_EQ(e.bottom, 0);
    EXPECT_FLOAT_EQ(e.left, 0);
}

TEST(EdgeInsets, UniformConstruction) {
    EdgeInsets e(10);
    EXPECT_FLOAT_EQ(e.top, 10);
    EXPECT_FLOAT_EQ(e.right, 10);
    EXPECT_FLOAT_EQ(e.bottom, 10);
    EXPECT_FLOAT_EQ(e.left, 10);
}

TEST(EdgeInsets, HorizontalVertical) {
    EdgeInsets e(10, 20, 10, 20);
    EXPECT_FLOAT_EQ(e.horizontal(), 40);
    EXPECT_FLOAT_EQ(e.vertical(), 20);
}

// =========================================================================
// CornerRadii Tests
// =========================================================================

TEST(CornerRadii, DefaultConstruction) {
    CornerRadii cr;
    EXPECT_FLOAT_EQ(cr.topLeft, 0);
    EXPECT_FLOAT_EQ(cr.topRight, 0);
    EXPECT_FLOAT_EQ(cr.bottomRight, 0);
    EXPECT_FLOAT_EQ(cr.bottomLeft, 0);
}

TEST(CornerRadii, UniformConstruction) {
    CornerRadii cr(8);
    EXPECT_TRUE(cr.isUniform());
    EXPECT_FLOAT_EQ(cr.topLeft, 8);
}

TEST(CornerRadii, NonUniform) {
    CornerRadii cr(1, 2, 3, 4);
    EXPECT_FALSE(cr.isUniform());
}

// =========================================================================
// Layer Tests (no GPU context)
// =========================================================================

TEST(Layer, DefaultState) {
    Layer layer;
    EXPECT_TRUE(layer.isVisible());
    EXPECT_EQ(layer.zIndex(), 0);
    EXPECT_FLOAT_EQ(layer.opacity(), 1.0f);
    EXPECT_FALSE(layer.isDirty());
    EXPECT_EQ(layer.rootWidget(), nullptr);
}

TEST(Layer, ZIndex) {
    Layer layer;
    layer.setZIndex(5);
    EXPECT_EQ(layer.zIndex(), 5);
}

TEST(Layer, Visibility) {
    Layer layer;
    layer.setVisible(false);
    EXPECT_FALSE(layer.isVisible());
    layer.setVisible(true);
    EXPECT_TRUE(layer.isVisible());
}

TEST(Layer, Opacity) {
    Layer layer;
    layer.setOpacity(0.5f);
    EXPECT_FLOAT_EQ(layer.opacity(), 0.5f);
}

TEST(Layer, DamageTracking) {
    Layer layer;
    EXPECT_FALSE(layer.isDirty());
    layer.invalidate(Rect(0, 0, 10, 10));
    EXPECT_TRUE(layer.isDirty());
    EXPECT_EQ(layer.damageRects().size(), 1u);
    layer.clearDamage();
    EXPECT_FALSE(layer.isDirty());
}

TEST(Layer, InvalidateAll) {
    Layer layer;
    layer.setBounds(Rect(0, 0, 800, 600));
    layer.invalidateAll();
    EXPECT_TRUE(layer.isDirty());
    EXPECT_EQ(layer.damageRects().size(), 1u);
    EXPECT_FLOAT_EQ(layer.damageRects()[0].width, 800);
}

TEST(Layer, Bounds) {
    Layer layer;
    layer.setBounds(Rect(10, 20, 300, 400));
    EXPECT_FLOAT_EQ(layer.bounds().x, 10);
    EXPECT_FLOAT_EQ(layer.bounds().y, 20);
    EXPECT_FLOAT_EQ(layer.bounds().width, 300);
    EXPECT_FLOAT_EQ(layer.bounds().height, 400);
}

// =========================================================================
// Compositor Tests (no GPU)
// =========================================================================

TEST(Compositor, InitShutdownNoGpu) {
    Compositor comp;
    EXPECT_FALSE(comp.init(nullptr));
    comp.shutdown();
}

TEST(Compositor, CreateDestroyLayer) {
    Compositor comp;
    EXPECT_EQ(comp.layers().size(), 0u);
    Layer* l1 = comp.createLayer();
    EXPECT_NE(l1, nullptr);
    EXPECT_EQ(comp.layers().size(), 1u);
    Layer* l2 = comp.createLayer();
    EXPECT_EQ(comp.layers().size(), 2u);
    comp.destroyLayer(l1);
    EXPECT_EQ(comp.layers().size(), 1u);
    comp.destroyLayer(l2);
    EXPECT_EQ(comp.layers().size(), 0u);
}

TEST(Compositor, MoveLayerZIndex) {
    Compositor comp;
    Layer* l1 = comp.createLayer();
    Layer* l2 = comp.createLayer();
    comp.moveLayer(l1, 10);
    comp.moveLayer(l2, 5);
    EXPECT_EQ(l1->zIndex(), 10);
    EXPECT_EQ(l2->zIndex(), 5);
    comp.shutdown();
}

TEST(Compositor, BackgroundColor) {
    Compositor comp;
    comp.setBackgroundColor(Color::black());
    EXPECT_EQ(comp.backgroundColor(), Color::black());
}

TEST(Compositor, VsyncToggle) {
    Compositor comp;
    EXPECT_TRUE(comp.vsyncEnabled());
    comp.setVsyncEnabled(false);
    EXPECT_FALSE(comp.vsyncEnabled());
}

// =========================================================================
// Widget Tests
// =========================================================================

TEST(Widget, DefaultState) {
    Widget w;
    EXPECT_TRUE(w.isVisible());
    EXPECT_TRUE(w.isEnabled());
    EXPECT_FALSE(w.isFocused());
    EXPECT_FALSE(w.isHovered());
    EXPECT_FALSE(w.isPressed());
    EXPECT_GT(w.id(), 0u);
}

TEST(Widget, SetBounds) {
    Widget w;
    w.setBounds(Rect(10, 20, 100, 200));
    EXPECT_FLOAT_EQ(w.bounds().x, 10);
    EXPECT_FLOAT_EQ(w.bounds().y, 20);
    EXPECT_FLOAT_EQ(w.bounds().width, 100);
    EXPECT_FLOAT_EQ(w.bounds().height, 200);
}

TEST(Widget, SetPosition) {
    Widget w;
    w.setSize(50, 60);
    w.setPosition(15, 25);
    EXPECT_FLOAT_EQ(w.bounds().x, 15);
    EXPECT_FLOAT_EQ(w.bounds().y, 25);
    EXPECT_FLOAT_EQ(w.bounds().width, 50);
    EXPECT_FLOAT_EQ(w.bounds().height, 60);
}

TEST(Widget, Visibility) {
    Widget w;
    w.setVisible(false);
    EXPECT_FALSE(w.isVisible());
}

TEST(Widget, EnableDisable) {
    Widget w;
    w.setEnabled(false);
    EXPECT_FALSE(w.isEnabled());
}

TEST(Widget, Focus) {
    Widget w;
    w.setFocused(true);
    EXPECT_TRUE(w.isFocused());
}

TEST(Widget, BackgroundColor) {
    Widget w;
    w.setBackgroundColor(Color::red());
    EXPECT_EQ(w.backgroundColor(), Color::red());
}

TEST(Widget, PaddingMargin) {
    Widget w;
    w.setPadding(EdgeInsets(5));
    w.setMargin(EdgeInsets(10));
    EXPECT_FLOAT_EQ(w.padding().top, 5);
    EXPECT_FLOAT_EQ(w.margin().left, 10);
}

TEST(Widget, AddRemoveChildren) {
    Widget parent;
    auto child1 = std::make_unique<Widget>();
    auto child2 = std::make_unique<Widget>();
    Widget* c1ptr = child1.get();
    Widget* c2ptr = child2.get();

    parent.addChild(std::move(child1));
    parent.addChild(std::move(child2));
    EXPECT_EQ(parent.children().size(), 2u);
    EXPECT_EQ(parent.children()[0]->parent(), &parent);

    parent.removeChild(c1ptr);
    EXPECT_EQ(parent.children().size(), 1u);
    EXPECT_EQ(parent.children()[0].get(), c2ptr);

    parent.clearChildren();
    EXPECT_EQ(parent.children().size(), 0u);
}

TEST(Widget, HitTest) {
    Widget parent;
    parent.setBounds(Rect(0, 0, 200, 200));

    auto child = std::make_unique<Widget>();
    child->setBounds(Rect(50, 50, 50, 50));
    Widget* cptr = child.get();
    parent.addChild(std::move(child));

    Widget* hit = parent.hitTest(75, 75);
    EXPECT_EQ(hit, cptr);

    Widget* hitParent = parent.hitTest(10, 10);
    EXPECT_EQ(hitParent, &parent);

    Widget* miss = parent.hitTest(300, 300);
    EXPECT_EQ(miss, nullptr);
}

TEST(Widget, HitTestInvisible) {
    Widget w;
    w.setBounds(Rect(0, 0, 100, 100));
    w.setVisible(false);
    EXPECT_EQ(w.hitTest(50, 50), nullptr);
}

TEST(Widget, UniqueIds) {
    Widget w1;
    Widget w2;
    Widget w3;
    EXPECT_NE(w1.id(), w2.id());
    EXPECT_NE(w2.id(), w3.id());
}

// =========================================================================
// Container Tests
// =========================================================================

TEST(Container, ChildCount) {
    Container cont;
    EXPECT_EQ(cont.children().size(), 0u);
    cont.addChild(std::make_unique<Widget>());
    cont.addChild(std::make_unique<Widget>());
    EXPECT_EQ(cont.children().size(), 2u);
}

// =========================================================================
// Event Tests
// =========================================================================

TEST(Event, DefaultType) {
    Event e;
    EXPECT_EQ(e.type, EventType::None);
}

TEST(Event, IsMouseCheck) {
    Event e;
    e.type = EventType::MouseDown;
    EXPECT_TRUE(e.isMouse());
    EXPECT_FALSE(e.isKeyboard());
}

TEST(Event, IsKeyboardCheck) {
    Event e;
    e.type = EventType::KeyDown;
    EXPECT_TRUE(e.isKeyboard());
    EXPECT_FALSE(e.isMouse());
}

TEST(Event, ResizeType) {
    Event e;
    e.type = EventType::Resize;
    e.window.width = 1920;
    e.window.height = 1080;
    EXPECT_EQ(e.window.width, 1920);
    EXPECT_EQ(e.window.height, 1080);
}

// =========================================================================
// Easing Tests
// =========================================================================

TEST(Easing, LinearEndpoints) {
    EXPECT_FLOAT_EQ(Easing::linear(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(Easing::linear(1.0f), 1.0f);
}

TEST(Easing, LinearMidpoint) {
    EXPECT_FLOAT_EQ(Easing::linear(0.5f), 0.5f);
}

TEST(Easing, EaseInEndpoints) {
    EXPECT_FLOAT_EQ(Easing::easeIn(0.0f), 0.0f);
    EXPECT_NEAR(Easing::easeIn(1.0f), 1.0f, 0.01f);
}

TEST(Easing, EaseOutEndpoints) {
    EXPECT_FLOAT_EQ(Easing::easeOut(0.0f), 0.0f);
    EXPECT_NEAR(Easing::easeOut(1.0f), 1.0f, 0.01f);
}

TEST(Easing, EaseInOutEndpoints) {
    EXPECT_FLOAT_EQ(Easing::easeInOut(0.0f), 0.0f);
    EXPECT_NEAR(Easing::easeInOut(1.0f), 1.0f, 0.01f);
}

TEST(Easing, EaseInSlowerStart) {
    // easeIn should be slower at start than linear
    EXPECT_LT(Easing::easeIn(0.25f), 0.25f);
}

TEST(Easing, EaseOutFasterStart) {
    // easeOut should be faster at start than linear
    EXPECT_GT(Easing::easeOut(0.25f), 0.25f);
}

// =========================================================================
// Animation Tests
// =========================================================================

TEST(Animation, InitialState) {
    Animation anim;
    EXPECT_EQ(anim.state(), AnimationState::Idle);
}

TEST(Animation, StartChangesState) {
    Animation anim;
    anim.from(0).to(100).duration(300);
    float captured = -1;
    anim.setter([&captured](float v) { captured = v; });
    anim.start();
    EXPECT_EQ(anim.state(), AnimationState::Running);
}

TEST(Animation, UpdateProgresses) {
    Animation anim;
    float captured = -1;
    anim.from(0).to(100).duration(100)
        .easing(Easing::linear)
        .setter([&captured](float v) { captured = v; });
    anim.start();
    bool running = anim.update(50);
    EXPECT_TRUE(running);
    EXPECT_NEAR(captured, 50, 5);
}

TEST(Animation, CompleteAfterDuration) {
    Animation anim;
    float captured = -1;
    anim.from(0).to(100).duration(100)
        .easing(Easing::linear)
        .setter([&captured](float v) { captured = v; });
    anim.start();
    bool running = anim.update(150);
    EXPECT_FALSE(running);
    EXPECT_NEAR(captured, 100, 1);
    EXPECT_EQ(anim.state(), AnimationState::Completed);
}

// =========================================================================
// FlexLayout Tests
// =========================================================================

TEST(FlexLayout, DefaultDirection) {
    FlexLayout flex;
    EXPECT_EQ(flex.direction, FlexDirection::Row);
}

TEST(FlexLayout, MeasureEmpty) {
    FlexLayout flex;
    std::vector<Widget*> children;
    Size s = flex.measure(children, Size(100, 100));
    EXPECT_FLOAT_EQ(s.width, 0);
    EXPECT_FLOAT_EQ(s.height, 0);
}

// =========================================================================
// Theme Tests
// =========================================================================

TEST(Theme, LightThemeCreation) {
    Theme light = Theme::light();
    EXPECT_EQ(light.name, "Light");
}

TEST(Theme, DarkThemeCreation) {
    Theme dark = Theme::dark();
    EXPECT_EQ(dark.name, "Dark");
}
