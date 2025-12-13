#pragma once

/**
 * @file lexer.hpp
 * @brief JavaScript tokenizer
 */

#include "token.hpp"
#include "source_code.hpp"
#include <memory>
#include <vector>
#include <string>

namespace Zepra::Frontend {

/**
 * @brief JavaScript lexer/tokenizer
 * 
 * Converts JavaScript source code into a stream of tokens.
 */
class Lexer {
public:
    /**
     * @brief Create a lexer for the given source
     */
    explicit Lexer(const SourceCode* source);
    
    /**
     * @brief Get the next token
     */
    Token nextToken();
    
    /**
     * @brief Peek at the next token without consuming it
     */
    const Token& peek();
    
    /**
     * @brief Check if at end of file
     */
    bool isEof() const { return offset_ >= source_->length(); }
    
    /**
     * @brief Get current position
     */
    const SourceLocation& currentLocation() const { return location_; }
    
    /**
     * @brief Get all errors encountered
     */
    const std::vector<std::string>& errors() const { return errors_; }
    
    /**
     * @brief Check if any errors occurred
     */
    bool hasErrors() const { return !errors_.empty(); }
    
private:
    // Character reading
    char current() const;
    char peek(size_t ahead) const;
    char advance();
    bool match(char expected);
    void skipWhitespace();
    void skipLineComment();
    void skipBlockComment();
    
    // Token scanning
    Token scanNumber();
    Token scanString(char quote);
    Token scanIdentifierOrKeyword();
    Token scanRegExp();
    Token scanTemplate();
    Token scanOperator();
    
    // Utilities
    Token makeToken(TokenType type);
    Token makeToken(TokenType type, std::string value);
    Token errorToken(const std::string& message);
    void addError(const std::string& message);
    
    bool isDigit(char c) const;
    bool isHexDigit(char c) const;
    bool isAlpha(char c) const;
    bool isAlphaNumeric(char c) const;
    bool isIdentifierStart(char c) const;
    bool isIdentifierPart(char c) const;
    
    const SourceCode* source_;
    size_t offset_ = 0;
    SourceLocation location_;
    SourceLocation tokenStart_;
    
    Token peekedToken_;
    bool hasPeeked_ = false;
    
    std::vector<std::string> errors_;
    
    // Context for regex vs division disambiguation
    bool canFollowDivision_ = false;
};

} // namespace Zepra::Frontend
