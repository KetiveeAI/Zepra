/**
 * @file nx_pdf_content.h
 * @brief Content stream interpreter boundaries
 */

#pragma once

#include "nx_pdf_graphics.h"
#include "../parser/nx_pdf_lexer.h"
#include "../parser/nx_pdf_document.h"
#include "../fonts/nx_pdf_tounicode.h"
#include <string_view>
#include <string>
#include <unordered_map>
#include <vector>

namespace nxrender {
namespace pdf {
namespace renderer {

struct TextState {
    Matrix tlm; // Text line matrix
    Matrix tm;  // Text matrix
    double charSpace = 0.0; // Tc
    double wordSpace = 0.0; // Tw
    double leading = 0.0;   // Tl
    double fontSize = 12.0; 
    std::string fontId;
};

class ContentInterpreter {
public:
    explicit ContentInterpreter(std::string_view streamData, parser::PdfDictionary* resources = nullptr, parser::XRefTable* xref = nullptr);
    
    // Exectue operators
    void Interpret();

    const std::string& GetExtractedText() const { return extractedText_; }

private:
    void ExecuteOperator(std::string_view op);
    double PopNumber();
    std::string PopString();
    void LoadFontCMap(const std::string& fontId);
    std::string ApplyCMap(const std::string& rawText, const std::string& fontId);

    parser::Lexer lexer_;
    GraphicsStateStack stateStack_;
    
    parser::PdfDictionary* resources_;
    parser::XRefTable* xref_;
    std::unordered_map<std::string, std::unique_ptr<fonts::ToUnicodeCMap>> cmaps_;
    std::unordered_map<std::string, std::string> fontEncodings_;

    // Text Isolation Machine post-fix layout mechanics strictly
    std::vector<parser::Token> operandStack_;
    Point currentPoint_;
    
    // Text Isolation Machine
    TextState textState_;
    bool inTextObject_ = false;
    std::string extractedText_;
};

} // namespace renderer
} // namespace pdf
} // namespace nxrender
