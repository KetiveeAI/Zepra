/**
 * @file nx_pdf_tounicode.cpp
 * @brief CMap implementation buffers explicitly isolated
 */

#include "nx_pdf_tounicode.h"
#include "../parser/nx_pdf_lexer.h"
#include <string_view>

namespace nxrender {
namespace pdf {
namespace fonts {

ToUnicodeCMap::ToUnicodeCMap(const std::vector<uint8_t>& cmapBuffer) {
    if (!cmapBuffer.empty()) {
        ParseBuffer(cmapBuffer);
    }
}

std::string ToUnicodeCMap::MapCodeToUnicode(int code) const {
    auto it = mapping_.find(code);
    if (it != mapping_.end()) {
        return it->second;
    }
    // Hard fallback mappings directly encoding ASCII/WinAnsi limits bounds safely
    return std::string(1, static_cast<char>(code));
}

void ToUnicodeCMap::ParseBuffer(const std::vector<uint8_t>& cmapBuffer) {
    std::string_view buffer(reinterpret_cast<const char*>(cmapBuffer.data()), cmapBuffer.size());
    parser::Lexer lexer(buffer);
    
    auto DecodeToUTF8 = [](const std::string& hexStr) {
        std::string utf8_full;
        for (size_t ptr = 0; ptr + 1 < hexStr.size(); ptr += 2) {
            int utf16 = (static_cast<uint8_t>(hexStr[ptr]) << 8) | static_cast<uint8_t>(hexStr[ptr+1]);
            if (utf16 <= 0x7F) utf8_full += static_cast<char>(utf16);
            else if (utf16 <= 0x7FF) {
                utf8_full += static_cast<char>(0xC0 | ((utf16 >> 6) & 0x1F));
                utf8_full += static_cast<char>(0x80 | (utf16 & 0x3F));
            } else {
                utf8_full += static_cast<char>(0xE0 | ((utf16 >> 12) & 0x0F));
                utf8_full += static_cast<char>(0x80 | ((utf16 >> 6) & 0x3F));
                utf8_full += static_cast<char>(0x80 | (utf16 & 0x3F));
            }
        }
        return utf8_full;
    };
    
    // Parse mapping tokens sequentially isolating ranges
    while (true) {
        parser::Token t = lexer.NextToken();
        if (t.type == parser::TokenType::EndOfFile) break;
        
        if (t.type == parser::TokenType::Keyword) {
            if (t.lexeme == "beginbfchar") {
                while (true) {
                    parser::Token src = lexer.NextToken();
                    if (src.type == parser::TokenType::Keyword && src.lexeme == "endbfchar") break;
                    parser::Token dst = lexer.NextToken();
                    if (src.type == parser::TokenType::HexString && dst.type == parser::TokenType::HexString) {
                        int code = 0;
                        if (src.stringValue.size() == 2) isTwoByte_ = true;
                        for (char c : src.stringValue) code = (code << 8) | static_cast<uint8_t>(c);
                        
                        mapping_[code] = DecodeToUTF8(dst.stringValue);
                    }
                }
            } else if (t.lexeme == "beginbfrange") {
                while (true) {
                    parser::Token startSrc = lexer.NextToken();
                    if (startSrc.type == parser::TokenType::Keyword && startSrc.lexeme == "endbfrange") break;
                    parser::Token endSrc = lexer.NextToken();
                    parser::Token dst = lexer.NextToken();
                    
                    if (startSrc.type == parser::TokenType::HexString && endSrc.type == parser::TokenType::HexString) {
                        int codeStart = 0;
                        if (startSrc.stringValue.size() == 2) isTwoByte_ = true;
                        
                        for (char c : startSrc.stringValue) codeStart = (codeStart << 8) | static_cast<uint8_t>(c);
                        int codeEnd = 0;
                        for (char c : endSrc.stringValue) codeEnd = (codeEnd << 8) | static_cast<uint8_t>(c);
                        
                        if (dst.type == parser::TokenType::HexString) {
                            if (dst.stringValue.size() >= 2) {
                                std::string baseStr = dst.stringValue;
                                // Increment the last character in the string for the range mapped
                                for (int c = codeStart; c <= codeEnd; ++c) {
                                    mapping_[c] = DecodeToUTF8(baseStr);
                                    if (baseStr.size() >= 2) {
                                        uint8_t lastByte = static_cast<uint8_t>(baseStr.back());
                                        baseStr[baseStr.size() - 1] = static_cast<char>(lastByte + 1);
                                    }
                                }
                            }
                        } else if (dst.type == parser::TokenType::ArrayStart) {
                            for (int c = codeStart; c <= codeEnd; ++c) {
                                parser::Token arrItem = lexer.NextToken();
                                if (arrItem.type == parser::TokenType::HexString) {
                                    mapping_[c] = DecodeToUTF8(arrItem.stringValue);
                                }
                            }
                            parser::Token arrEnd = lexer.NextToken(); // skip ]
                        }
                    }
                }
            }
        }
    }
}

} // namespace fonts
} // namespace pdf
} // namespace nxrender
