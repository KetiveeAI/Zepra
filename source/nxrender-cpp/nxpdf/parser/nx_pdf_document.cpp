/**
 * @file nx_pdf_document.cpp
 * @brief Top-level document model parser implementation
 */

#include "nx_pdf_document.h"
#include <zlib.h>

namespace nxrender {
namespace pdf {
namespace parser {

std::vector<uint8_t> DecodeFlateData(const std::vector<uint8_t>& in) {
    if (in.empty()) return {};
    
    z_stream strm = {};
    if (inflateInit(&strm) != Z_OK) return {};
    
    strm.next_in = const_cast<Bytef*>(in.data());
    strm.avail_in = in.size();
    
    std::vector<uint8_t> out;
    std::vector<uint8_t> chunk(32768);
    
    do {
        strm.next_out = chunk.data();
        strm.avail_out = chunk.size();
        
        int ret = inflate(&strm, Z_NO_FLUSH);
        // Clean exit strictly handling unsupported format states
        if (ret == Z_STREAM_ERROR || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
            inflateEnd(&strm);
            return {};
        }
        
        size_t have = chunk.size() - strm.avail_out;
        out.insert(out.end(), chunk.data(), chunk.data() + have);
        
        if (ret == Z_STREAM_END) break;
    } while (strm.avail_out == 0);
    
    inflateEnd(&strm);
    return out;
}

PdfDocumentParser::PdfDocumentParser(std::string_view data) 
    : fileData_(data), lexer_(data), xrefTable_(this) {}

PdfErrorCode PdfDocumentParser::Parse() {
    size_t startXRefOffset = 0;
    
    PdfErrorCode err = ReadTrailerAndStartXRef(startXRefOffset);
    if (err != PdfErrorCode::None) return err;
    
    return ParseXRefSection(startXRefOffset);
}

PdfErrorCode PdfDocumentParser::ReadTrailerAndStartXRef(size_t& outXRefOffset) {
    if (fileData_.size() < 30) return PdfErrorCode::InvalidFile;
    
    // Seek backwards from end of file sequentially to find "startxref"
    size_t scanIndex = fileData_.size() - 10;
    bool found = false;
    
    while (scanIndex > 0 && (fileData_.size() - scanIndex) < 1024) {
        if (fileData_.substr(scanIndex, 9) == "startxref") {
            found = true;
            break;
        }
        scanIndex--;
    }
    
    if (!found) return PdfErrorCode::NoTrailer;
    
    lexer_.SetPosition(scanIndex + 9);
    Token numToken = lexer_.NextToken();
    if (numToken.type != TokenType::Integer) return PdfErrorCode::ParseError;
    
    outXRefOffset = static_cast<size_t>(numToken.intValue);
    return PdfErrorCode::None;
}

PdfErrorCode PdfDocumentParser::ParseXRefSection(size_t startXRefOffset) {
    lexer_.SetPosition(startXRefOffset);
    
    Token keyword = lexer_.NextToken();
    if (keyword.type != TokenType::Keyword || keyword.lexeme != "xref") {
        return PdfErrorCode::ParseError;
    }
    
    // Simplistic xref sequence indexing parser
    while (true) {
        Token firstObj = lexer_.NextToken();
        if (firstObj.type == TokenType::Keyword && firstObj.lexeme == "trailer") {
            break; // Done with xref table entries
        }
        
        if (firstObj.type != TokenType::Integer) return PdfErrorCode::ParseError;
        
        Token countToken = lexer_.NextToken();
        if (countToken.type != TokenType::Integer) return PdfErrorCode::ParseError;
        
        int startId = firstObj.intValue;
        int count = countToken.intValue;
        
        for (int i = 0; i < count; i++) {
            Token offsetTok = lexer_.NextToken();
            Token genTok = lexer_.NextToken();
            Token stateTok = lexer_.NextToken();
            
            if (offsetTok.type != TokenType::Integer || genTok.type != TokenType::Integer || stateTok.type != TokenType::Keyword) {
                return PdfErrorCode::ParseError;
            }
            
            XRefEntryType type = XRefEntryType::Free;
            if (stateTok.lexeme == "n") {
                type = XRefEntryType::InUse;
                // Basic PDF 1.4 bounds and logic assignment
                xrefTable_.RegisterObject(startId + i, genTok.intValue, static_cast<size_t>(offsetTok.intValue), type);
            }
        }
    }
    
    size_t savedDictPos = lexer_.GetPosition();
    Token TrailerDictBegin = lexer_.NextToken();
    if (TrailerDictBegin.type == TokenType::DictStart) {
        lexer_.SetPosition(savedDictPos);
        auto parsedTrailer = ParseExpression();
        if (parsedTrailer && parsedTrailer->GetType() == PdfObjectType::Dictionary) {
            // trailer_.reset(static_cast<PdfDictionary*>(parsedTrailer.release()));
            // Cast requires specific conversion logic due to polymorphic unique_ptr graph
            auto rootDict = std::unique_ptr<PdfDictionary>(static_cast<PdfDictionary*>(parsedTrailer.release()));
            trailer_ = std::move(rootDict);
        }
    }
    
    return PdfErrorCode::None;
}

std::unique_ptr<PdfObject> PdfDocumentParser::ParseObjectAtOffset(size_t offset) {
    lexer_.SetPosition(offset);
    
    Token objId = lexer_.NextToken();
    Token genId = lexer_.NextToken();
    Token objKeyword = lexer_.NextToken();
    
    if (objId.type != TokenType::Integer || genId.type != TokenType::Integer || (objKeyword.type != TokenType::Keyword && objKeyword.lexeme != "obj")) {
        return nullptr;
    }
    
    auto mappedExpression = ParseExpression();
    
    Token endKeyword = lexer_.NextToken();
    if (endKeyword.type != TokenType::Keyword || endKeyword.lexeme != "endobj") {
        // Technically malformed but object mapped successfully 
    }
    
    return mappedExpression;
}

std::unique_ptr<PdfObject> PdfDocumentParser::ParseExpression() {
    Token t = lexer_.NextToken();
    
    // Check for indirect references (e.g. 5 0 R)
    if (t.type == TokenType::Integer) {
        size_t savedPos = lexer_.GetPosition();
        Token t2 = lexer_.NextToken();
        if (t2.type == TokenType::Integer) {
            Token t3 = lexer_.NextToken();
            if (t3.type == TokenType::Keyword && t3.lexeme == "R") {
                return std::make_unique<PdfReference>(t.intValue, t2.intValue);
            }
        }
        // Backtrack
        lexer_.SetPosition(savedPos);
        return std::make_unique<PdfInteger>(t.intValue);
    }
    
    switch (t.type) {
        case TokenType::Null:
            return std::make_unique<PdfNull>();
        case TokenType::Boolean:
            return std::make_unique<PdfBool>(t.boolValue);
        case TokenType::Real:
            return std::make_unique<PdfReal>(t.realValue);
        case TokenType::StringLiteral:
        case TokenType::HexString:
            return std::make_unique<PdfString>(t.stringValue);
        case TokenType::Name:
            return std::make_unique<PdfName>(t.stringValue);
            
        case TokenType::ArrayStart: {
            auto arr = std::make_unique<PdfArray>();
            size_t savedPos = lexer_.GetPosition();
            Token peek = lexer_.NextToken();
            
            while (peek.type != TokenType::ArrayEnd && peek.type != TokenType::EndOfFile) {
                lexer_.SetPosition(savedPos);
                if (auto item = ParseExpression()) {
                    arr->items.push_back(std::move(item));
                } else {
                    break;
                }
                savedPos = lexer_.GetPosition();
                peek = lexer_.NextToken();
            }
            return arr;
        }
        case TokenType::DictStart: {
            auto dict = std::make_unique<PdfDictionary>();
            size_t savedPos = lexer_.GetPosition();
            Token peek = lexer_.NextToken();
            
            while (peek.type != TokenType::DictEnd && peek.type != TokenType::EndOfFile) {
                if (peek.type == TokenType::Name) {
                    std::string key = peek.stringValue;
                    if (auto val = ParseExpression()) {
                        dict->map[key] = std::move(val);
                    }
                } else {
                    break; // Malformed dictionary
                }
                savedPos = lexer_.GetPosition();
                peek = lexer_.NextToken();
            }
            
            // Check for following stream
            size_t streamCheckPos = lexer_.GetPosition();
            Token streamKw = lexer_.NextToken();
            if (streamKw.type == TokenType::Keyword && streamKw.lexeme == "stream") {
                size_t startOfData = lexer_.GetPosition();
                if (fileData_[startOfData] == '\r') startOfData++;
                if (fileData_[startOfData] == '\n') startOfData++;
                
                size_t endOfData = fileData_.find("endstream", startOfData);
                if (endOfData == std::string_view::npos) {
                    endOfData = fileData_.size();
                } else {
                    size_t trimEnd = endOfData;
                    if (trimEnd > startOfData && fileData_[trimEnd - 1] == '\n') trimEnd--;
                    if (trimEnd > startOfData && fileData_[trimEnd - 1] == '\r') trimEnd--;
                    
                    std::vector<uint8_t> streamData(fileData_.begin() + startOfData, fileData_.begin() + trimEnd);
                    lexer_.SetPosition(endOfData + 9);
                    
                    return std::make_unique<PdfStream>(std::move(dict), std::move(streamData));
                }
            } else {
                lexer_.SetPosition(streamCheckPos);
            }
            
            return dict;
        }
        
        case TokenType::EndOfFile:
        case TokenType::Error:
        default:
            return nullptr;
    }
}


} // namespace parser
} // namespace pdf
} // namespace nxrender
