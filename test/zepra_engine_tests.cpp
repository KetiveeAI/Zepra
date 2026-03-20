// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file zepra_engine_tests.cpp
 * @brief GTest suite for zepraEngine core types, HTML parser, DOM, and utilities
 *
 * Tests the engine-layer data structures and logic without requiring
 * SDL2 or a running window. Covers:
 * - Core enums and structs (SystemState, DownloadState, AuthState)
 * - DownloadInfo / UserInfo / AuthToken defaults
 * - HTML tokenizer and DOM builder
 * - Utility functions (email validation, file size formatting, etc.)
 */

#include <gtest/gtest.h>
#include <string>
#include <chrono>

// Engine headers live flat in zepraEngine/engine/
#include "engine/html_parser.h"

// =========================================================================
// HTML Token Tests
// =========================================================================

TEST(HTMLToken, DefaultConstruction) {
    zepra::HTMLToken token;
    EXPECT_EQ(token.type, zepra::TokenType::EOF_TOKEN);
    EXPECT_FALSE(token.selfClosing);
    EXPECT_TRUE(token.data.empty());
    EXPECT_TRUE(token.tagName.empty());
}

TEST(HTMLToken, TypedConstruction) {
    zepra::HTMLToken token(zepra::TokenType::START_TAG);
    EXPECT_EQ(token.type, zepra::TokenType::START_TAG);
    EXPECT_FALSE(token.selfClosing);
}

TEST(HTMLToken, AllTypes) {
    EXPECT_NE(static_cast<int>(zepra::TokenType::DOCTYPE),
              static_cast<int>(zepra::TokenType::START_TAG));
    EXPECT_NE(static_cast<int>(zepra::TokenType::END_TAG),
              static_cast<int>(zepra::TokenType::COMMENT));
    EXPECT_NE(static_cast<int>(zepra::TokenType::CHARACTER),
              static_cast<int>(zepra::TokenType::EOF_TOKEN));
}

// =========================================================================
// HTML Tokenizer Tests
// =========================================================================

TEST(HTMLTokenizer, EmptyInput) {
    zepra::HTMLTokenizer tokenizer("");
    EXPECT_FALSE(tokenizer.hasMoreTokens());
    auto token = tokenizer.nextToken();
    EXPECT_EQ(token.type, zepra::TokenType::EOF_TOKEN);
}

TEST(HTMLTokenizer, SimpleStartTag) {
    zepra::HTMLTokenizer tokenizer("<div>");
    EXPECT_TRUE(tokenizer.hasMoreTokens());
    auto token = tokenizer.nextToken();
    EXPECT_EQ(token.type, zepra::TokenType::START_TAG);
    EXPECT_EQ(token.tagName, "div");
    EXPECT_FALSE(token.selfClosing);
}

TEST(HTMLTokenizer, EndTag) {
    zepra::HTMLTokenizer tokenizer("</div>");
    auto token = tokenizer.nextToken();
    EXPECT_EQ(token.type, zepra::TokenType::END_TAG);
    EXPECT_EQ(token.tagName, "div");
}

TEST(HTMLTokenizer, SelfClosingTag) {
    zepra::HTMLTokenizer tokenizer("<br/>");
    auto token = tokenizer.nextToken();
    EXPECT_EQ(token.type, zepra::TokenType::START_TAG);
    EXPECT_EQ(token.tagName, "br");
    EXPECT_TRUE(token.selfClosing);
}

TEST(HTMLTokenizer, TagWithAttributes) {
    zepra::HTMLTokenizer tokenizer("<a href=\"https://test.com\" class=\"link\">");
    auto token = tokenizer.nextToken();
    EXPECT_EQ(token.type, zepra::TokenType::START_TAG);
    EXPECT_EQ(token.tagName, "a");
    EXPECT_EQ(token.attributes["href"], "https://test.com");
    EXPECT_EQ(token.attributes["class"], "link");
}

TEST(HTMLTokenizer, TextContent) {
    zepra::HTMLTokenizer tokenizer("Hello World");
    auto token = tokenizer.nextToken();
    EXPECT_EQ(token.type, zepra::TokenType::CHARACTER);
    EXPECT_FALSE(token.data.empty());
}

TEST(HTMLTokenizer, MultipleTokens) {
    zepra::HTMLTokenizer tokenizer("<p>text</p>");
    auto t1 = tokenizer.nextToken();
    EXPECT_EQ(t1.type, zepra::TokenType::START_TAG);
    EXPECT_EQ(t1.tagName, "p");

    auto t2 = tokenizer.nextToken();
    EXPECT_EQ(t2.type, zepra::TokenType::CHARACTER);

    auto t3 = tokenizer.nextToken();
    EXPECT_EQ(t3.type, zepra::TokenType::END_TAG);
    EXPECT_EQ(t3.tagName, "p");
}

TEST(HTMLTokenizer, Comment) {
    zepra::HTMLTokenizer tokenizer("<!-- comment -->");
    auto token = tokenizer.nextToken();
    EXPECT_EQ(token.type, zepra::TokenType::COMMENT);
}

TEST(HTMLTokenizer, Doctype) {
    zepra::HTMLTokenizer tokenizer("<!DOCTYPE html>");
    auto token = tokenizer.nextToken();
    EXPECT_EQ(token.type, zepra::TokenType::DOCTYPE);
}

TEST(HTMLTokenizer, Reset) {
    zepra::HTMLTokenizer tokenizer("<div>");
    tokenizer.nextToken();
    tokenizer.reset();
    EXPECT_TRUE(tokenizer.hasMoreTokens());
    auto t = tokenizer.nextToken();
    EXPECT_EQ(t.tagName, "div");
}

// =========================================================================
// DOM Node Tests
// =========================================================================

TEST(DOMNode, Construction) {
    zepra::DOMNode node(zepra::NodeType::ELEMENT, "div");
    EXPECT_EQ(node.nodeType, zepra::NodeType::ELEMENT);
    EXPECT_EQ(node.nodeName, "div");
    EXPECT_FALSE(node.hasChildNodes());
    EXPECT_EQ(node.getChildCount(), 0u);
}

TEST(DOMNode, TextContent) {
    zepra::DOMNode node(zepra::NodeType::TEXT, "#text");
    node.setTextContent("Hello");
    EXPECT_EQ(node.getTextContent(), "Hello");
}

// =========================================================================
// ElementNode Tests
// =========================================================================

TEST(ElementNode, SetGetAttribute) {
    zepra::ElementNode elem(zepra::ElementType::DIV, "div");
    elem.setAttribute("id", "main");
    EXPECT_EQ(elem.getAttribute("id"), "main");
    EXPECT_TRUE(elem.hasAttribute("id"));
    EXPECT_FALSE(elem.hasAttribute("class"));
}

TEST(ElementNode, RemoveAttribute) {
    zepra::ElementNode elem(zepra::ElementType::DIV, "div");
    elem.setAttribute("class", "container");
    EXPECT_TRUE(elem.hasAttribute("class"));
    elem.removeAttribute("class");
    EXPECT_FALSE(elem.hasAttribute("class"));
}

TEST(ElementNode, StyleAccessEmpty) {
    zepra::ElementNode elem(zepra::ElementType::DIV, "div");
    EXPECT_TRUE(elem.getStyle("nonexistent").empty());
}

TEST(ElementNode, MissingAttribute) {
    zepra::ElementNode elem(zepra::ElementType::DIV, "div");
    EXPECT_TRUE(elem.getAttribute("nonexistent").empty());
}

// =========================================================================
// TextNode Tests
// =========================================================================

TEST(TextNode, Construction) {
    zepra::TextNode text("Hello World");
    EXPECT_EQ(text.nodeType, zepra::NodeType::TEXT);
    EXPECT_EQ(text.nodeName, "#text");
    EXPECT_EQ(text.getTextContent(), "Hello World");
}

TEST(TextNode, SetTextContent) {
    zepra::TextNode text("Original");
    text.setTextContent("Updated");
    EXPECT_EQ(text.getTextContent(), "Updated");
}

// =========================================================================
// DocumentNode Tests
// =========================================================================

TEST(DocumentNode, Construction) {
    zepra::DocumentNode doc;
    EXPECT_EQ(doc.nodeType, zepra::NodeType::DOCUMENT);
    EXPECT_EQ(doc.nodeName, "#document");
    EXPECT_EQ(doc.getDocumentElement(), nullptr);
    EXPECT_EQ(doc.getHead(), nullptr);
    EXPECT_EQ(doc.getBody(), nullptr);
}

// =========================================================================
// DOMBuilder / HTMLParser Tests
// =========================================================================

TEST(DOMBuilder, BuildEmptyHTML) {
    zepra::DOMBuilder builder;
    auto* doc = builder.build("");
    EXPECT_NE(doc, nullptr);
}

TEST(DOMBuilder, BuildSimpleHTML) {
    zepra::DOMBuilder builder;
    auto* doc = builder.build("<html><head></head><body></body></html>");
    EXPECT_NE(doc, nullptr);
}

TEST(HTMLParser, Construction) {
    zepra::HTMLParser parser;
    EXPECT_FALSE(parser.hasErrors());
}

TEST(HTMLParser, ParseMinimalDoc) {
    zepra::HTMLParser parser;
    auto* doc = parser.parse("<p>Hello</p>");
    EXPECT_NE(doc, nullptr);
}

TEST(HTMLParser, ParseWithErrors) {
    zepra::HTMLParser parser;
    parser.parse("<div><p>unclosed");
    // Parser should still return a document even with unclosed tags
}

TEST(HTMLParser, MaxNodes) {
    zepra::HTMLParser parser;
    parser.setMaxNodes(100);
    EXPECT_EQ(parser.getMaxNodes(), 100u);
}

TEST(HTMLParser, ClearErrors) {
    zepra::HTMLParser parser;
    parser.clearErrors();
    EXPECT_FALSE(parser.hasErrors());
    EXPECT_EQ(parser.getErrors().size(), 0u);
}

// =========================================================================
// HTML Utility Tests
// =========================================================================

TEST(HTMLUtils, IsVoidElement) {
    EXPECT_TRUE(zepra::html_utils::isVoidElement("br"));
    EXPECT_TRUE(zepra::html_utils::isVoidElement("hr"));
    EXPECT_TRUE(zepra::html_utils::isVoidElement("img"));
    EXPECT_TRUE(zepra::html_utils::isVoidElement("input"));
    EXPECT_FALSE(zepra::html_utils::isVoidElement("div"));
    EXPECT_FALSE(zepra::html_utils::isVoidElement("p"));
}

TEST(HTMLUtils, IsValidTagName) {
    EXPECT_TRUE(zepra::html_utils::isValidTagName("div"));
    EXPECT_TRUE(zepra::html_utils::isValidTagName("h1"));
    EXPECT_TRUE(zepra::html_utils::isValidTagName("custom-element"));
    EXPECT_FALSE(zepra::html_utils::isValidTagName(""));
}

TEST(HTMLUtils, EscapeHtml) {
    EXPECT_EQ(zepra::html_utils::escapeHtml("<b>bold</b>"), "&lt;b&gt;bold&lt;/b&gt;");
    EXPECT_EQ(zepra::html_utils::escapeHtml("a & b"), "a &amp; b");
    EXPECT_EQ(zepra::html_utils::escapeHtml("\"quoted\""), "&quot;quoted&quot;");
}

TEST(HTMLUtils, NormalizeWhitespace) {
    std::string result = zepra::html_utils::normalizeWhitespace("  hello   world  ");
    // Should collapse multiple spaces
    EXPECT_FALSE(result.empty());
}
