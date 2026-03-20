// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
#pragma once

#include "../common/types.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace zepra {

// DOM Node Types
enum class NodeType {
    ELEMENT,
    TEXT,
    COMMENT,
    DOCUMENT,
    DOCTYPE
};

// DOM Node Structure
struct DOMNode {
    NodeType type;
    String tagName;
    String textContent;
    std::unordered_map<String, String> attributes;
    std::vector<std::shared_ptr<DOMNode>> children;
    std::weak_ptr<DOMNode> parent;
    
    DOMNode(NodeType t = NodeType::ELEMENT) : type(t) {}
    
    // Helper methods
    String getAttribute(const String& name) const;
    void setAttribute(const String& name, const String& value);
    bool hasAttribute(const String& name) const;
    void appendChild(std::shared_ptr<DOMNode> child);
    std::vector<std::shared_ptr<DOMNode>> getElementsByTagName(const String& tag) const;
    std::shared_ptr<DOMNode> getElementById(const String& id) const;
    std::vector<std::shared_ptr<DOMNode>> getElementsByClassName(const String& className) const;
};

// HTML Parser with modern capabilities
class HTMLParserModern {
public:
    HTMLParserModern();
    ~HTMLParserModern() = default;
    
    // Parse HTML content
    std::shared_ptr<DOMNode> parse(const String& html);
    std::shared_ptr<DOMNode> parseFragment(const String& html);
    
    // Extract information
    std::vector<String> extractLinks(const String& html);
    std::vector<String> extractImages(const String& html);
    std::vector<String> extractScripts(const String& html);
    std::vector<String> extractStyles(const String& html);
    String extractTitle(const String& html);
    String extractMetaDescription(const String& html);
    std::unordered_map<String, String> extractMetaTags(const String& html);
    
    // Content extraction
    String extractTextContent(const String& html);
    String extractMainContent(const String& html);
    String extractArticleContent(const String& html);
    
    // Validation
    bool isValidHTML(const String& html);
    std::vector<String> validateHTML(const String& html);
    
    // Sanitization
    String sanitizeHTML(const String& html);
    String stripTags(const String& html);
    String escapeHTML(const String& html);
    String unescapeHTML(const String& html);
    
    // DOM manipulation
    String serialize(std::shared_ptr<DOMNode> node);
    String prettify(std::shared_ptr<DOMNode> node, int indent = 0);
    
    // Configuration
    void setStrictMode(bool strict) { strictMode_ = strict; }
    bool isStrictMode() const { return strictMode_; }
    
    void setMaxDepth(int depth) { maxDepth_ = depth; }
    int getMaxDepth() const { return maxDepth_; }
    
private:
    bool strictMode_;
    int maxDepth_;
    int currentDepth_;
    
    // Parsing helpers
    std::shared_ptr<DOMNode> parseElement(const String& html, size_t& pos);
    std::shared_ptr<DOMNode> parseText(const String& html, size_t& pos);
    std::shared_ptr<DOMNode> parseComment(const String& html, size_t& pos);
    std::shared_ptr<DOMNode> parseDoctype(const String& html, size_t& pos);
    
    // Token parsing
    String parseTagName(const String& html, size_t& pos);
    std::unordered_map<String, String> parseAttributes(const String& html, size_t& pos);
    String parseAttributeValue(const String& html, size_t& pos);
    
    // Utility methods
    void skipWhitespace(const String& html, size_t& pos);
    bool isWhitespace(char c);
    bool isAlpha(char c);
    bool isAlphaNum(char c);
    bool isValidTagName(const String& name);
    bool isSelfClosingTag(const String& name);
    bool isVoidElement(const String& name);
    
    // HTML entities
    String decodeEntities(const String& text);
    String encodeEntities(const String& text);
    
    // Error handling
    std::vector<String> errors_;
    void addError(const String& error);
};

// HTML5 Parser (more advanced)
class HTML5Parser {
public:
    HTML5Parser();
    ~HTML5Parser() = default;
    
    // Parse with HTML5 spec compliance
    std::shared_ptr<DOMNode> parse(const String& html);
    
    // Tokenization
    struct Token {
        enum class Type {
            DOCTYPE,
            START_TAG,
            END_TAG,
            COMMENT,
            CHARACTER,
            END_OF_FILE
        };
        
        Type type;
        String data;
        std::unordered_map<String, String> attributes;
        bool selfClosing;
        
        Token(Type t = Type::CHARACTER) : type(t), selfClosing(false) {}
    };
    
    std::vector<Token> tokenize(const String& html);
    std::shared_ptr<DOMNode> buildTree(const std::vector<Token>& tokens);
    
    // Error recovery
    void enableErrorRecovery(bool enable) { errorRecovery_ = enable; }
    bool isErrorRecoveryEnabled() const { return errorRecovery_; }
    
private:
    bool errorRecovery_;
    std::vector<String> errors_;
    
    // Tokenizer states
    enum class State {
        DATA,
        TAG_OPEN,
        TAG_NAME,
        BEFORE_ATTRIBUTE_NAME,
        ATTRIBUTE_NAME,
        AFTER_ATTRIBUTE_NAME,
        BEFORE_ATTRIBUTE_VALUE,
        ATTRIBUTE_VALUE_DOUBLE_QUOTED,
        ATTRIBUTE_VALUE_SINGLE_QUOTED,
        ATTRIBUTE_VALUE_UNQUOTED,
        AFTER_ATTRIBUTE_VALUE,
        SELF_CLOSING_START_TAG,
        MARKUP_DECLARATION_OPEN,
        COMMENT_START,
        COMMENT,
        COMMENT_END
    };
    
    State state_;
    Token currentToken_;
    String currentAttribute_;
    String currentAttributeValue_;
    
    // Tokenizer methods
    void processCharacter(char c, std::vector<Token>& tokens);
    void emitToken(Token token, std::vector<Token>& tokens);
    
    // Tree construction
    std::vector<std::shared_ptr<DOMNode>> openElements_;
    void insertElement(std::shared_ptr<DOMNode> element);
    void closeElement(const String& tagName);
};

// Utility functions
namespace html_utils {
    // HTML entity encoding/decoding
    String encodeHTML(const String& text);
    String decodeHTML(const String& text);
    
    // URL extraction
    std::vector<String> extractURLs(const String& html);
    std::vector<String> extractAbsoluteURLs(const String& html, const String& baseUrl);
    
    // Content cleaning
    String removeScripts(const String& html);
    String removeStyles(const String& html);
    String removeComments(const String& html);
    String removeAttributes(const String& html, const std::vector<String>& attrs);
    
    // Text extraction
    String extractVisibleText(const String& html);
    String extractStructuredText(const String& html);
    
    // Minification
    String minifyHTML(const String& html);
    String beautifyHTML(const String& html);
    
    // Validation
    bool isValidTag(const String& tag);
    bool isBlockElement(const String& tag);
    bool isInlineElement(const String& tag);
    bool isVoidElement(const String& tag);
    
    // Conversion
    String htmlToText(const String& html);
    String htmlToMarkdown(const String& html);
    String markdownToHTML(const String& markdown);
}

} // namespace zepra
