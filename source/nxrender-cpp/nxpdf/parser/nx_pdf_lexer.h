/**
 * @file nx_pdf_lexer.h
 * @brief Tokenizer for PDF syntax
 */

#pragma once

#include <string_view>
#include <string>

namespace nxrender {
namespace pdf {
namespace parser {

enum class TokenType {
    Error,
    EndOfFile,
    Null,
    Boolean,
    Integer,
    Real,
    StringLiteral, // (...)
    HexString,     // <...>
    Name,          // /Name
    Keyword,       // obj, endobj, stream, endstream, etc.
    ArrayStart,    // [
    ArrayEnd,      // ]
    DictStart,     // <<
    DictEnd        // >>
};

struct Token {
    TokenType type;
    std::string_view lexeme; // The exact bytes of the token
    
    // Parsed components depending on the type
    int intValue = 0;
    double realValue = 0.0;
    bool boolValue = false;
    std::string stringValue; 
};

class Lexer {
public:
    explicit Lexer(std::string_view buffer);
    
    Token NextToken();
    void Reset();
    size_t GetPosition() const { return position_; }
    void SetPosition(size_t pos) { position_ = pos; }

private:
    void SkipWhitespaceAndComments();
    Token ParseNumberOrKeyword();
    Token ParseStringLiteral();
    Token ParseHexString();
    Token ParseName();
    
    // Checks if the byte is considered whitespace in PDF spec
    bool IsPdfWhitespace(char c) const;
    bool IsDelimiter(char c) const;

    std::string_view buffer_;
    size_t position_;
};

} // namespace parser
} // namespace pdf
} // namespace nxrender
