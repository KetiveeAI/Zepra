/**
 * @file nx_pdf_content.cpp
 * @brief Content stream interpreter executing post-fix maps 
 */

#include "nx_pdf_content.h"

namespace nxrender {
namespace pdf {
namespace renderer {

ContentInterpreter::ContentInterpreter(std::string_view streamData, parser::PdfDictionary* resources, parser::XRefTable* xref) 
    : lexer_(streamData), resources_(resources), xref_(xref) {}

void ContentInterpreter::Interpret() {
    while (true) {
        parser::Token t = lexer_.NextToken();
        if (t.type == parser::TokenType::EndOfFile || t.type == parser::TokenType::Error) {
            break;
        }

        if (t.type == parser::TokenType::Keyword) {
            ExecuteOperator(t.lexeme);
            operandStack_.clear(); // post-fix resolution clean
        } else {
            operandStack_.push_back(std::move(t));
        }
    }
}

double ContentInterpreter::PopNumber() {
    if (operandStack_.empty()) return 0.0;
    auto t = operandStack_.back();
    operandStack_.pop_back();
    if (t.type == parser::TokenType::Real) return t.realValue;
    if (t.type == parser::TokenType::Integer) return static_cast<double>(t.intValue);
    return 0.0;
}

std::string ContentInterpreter::PopString() {
    if (operandStack_.empty()) return "";
    auto t = operandStack_.back();
    operandStack_.pop_back();
    if (t.type == parser::TokenType::StringLiteral || t.type == parser::TokenType::HexString || t.type == parser::TokenType::Name) {
        return t.stringValue;
    }
    return "";
}

void ContentInterpreter::ExecuteOperator(std::string_view op) {
    if (op == "q") { // Save State
        stateStack_.Push();
    } else if (op == "Q") { // Pop state
        stateStack_.Pop();
    } else if (op == "cm") { // Apply Matrix
        if (operandStack_.size() >= 6) {
            Matrix m;
            m.f = PopNumber();
            m.e = PopNumber();
            m.d = PopNumber();
            m.c = PopNumber();
            m.b = PopNumber();
            m.a = PopNumber();
            stateStack_.Current().ctm.Concat(m);
        }
    } else if (op == "m") { // MoveTo
        if (operandStack_.size() >= 2) {
            double y = PopNumber();
            double x = PopNumber();
            currentPoint_ = stateStack_.Current().ctm.Transform({x, y});
            // Native nxgfx Move command bridges here
        }
    } else if (op == "l") { // LineTo
        if (operandStack_.size() >= 2) {
            double y = PopNumber();
            double x = PopNumber();
            Point target = stateStack_.Current().ctm.Transform({x, y});
            currentPoint_ = target;
            // Native nxgfx Line command bridges here
        }
    } else if (op == "c") { // Cubic Bezier
         // Evaluates native coordinates
    } else if (op == "h") { // Close Path
         // Closes subpath securely
    } else if (op == "re") { // Rectangle bounds (x, y, w, h)
         // Native execution map
    } else if (op == "S" || op == "s") {
         // Direct stroke dispatching into nxgfx boundaries
    } else if (op == "f" || op == "F") {
         // Fill trigger execution mapping
    } else if (op == "g") { // Grayscale Fill
        if (operandStack_.size() >= 1) {
            double gray = PopNumber();
            stateStack_.Current().fillColor = {gray, gray, gray};
        }
    } else if (op == "G") { // Grayscale Stroke
        if (operandStack_.size() >= 1) {
            double gray = PopNumber();
            stateStack_.Current().strokeColor = {gray, gray, gray};
        }
    } else if (op == "rg") { // RGB Fill
        if (operandStack_.size() >= 3) {
            double b = PopNumber();
            double g = PopNumber();
            double r = PopNumber();
            stateStack_.Current().fillColor = {r, g, b};
        }
    } else if (op == "RG") { // RGB Stroke
        if (operandStack_.size() >= 3) {
            double b = PopNumber();
            double g = PopNumber();
            double r = PopNumber();
            stateStack_.Current().strokeColor = {r, g, b};
        }
    } else if (op == "k") { // CMYK Fill
        if (operandStack_.size() >= 4) {
            double k = PopNumber();
            double y = PopNumber();
            double m = PopNumber();
            double c = PopNumber();
            stateStack_.Current().fillColor = CMYKToRGB(c, m, y, k);
        }
    } else if (op == "K") { // CMYK Stroke
        if (operandStack_.size() >= 4) {
            double k = PopNumber();
            double y = PopNumber();
            double m = PopNumber();
            double c = PopNumber();
            stateStack_.Current().strokeColor = CMYKToRGB(c, m, y, k);
        }
    } else if (op == "BT") {
        inTextObject_ = true;
        textState_.tlm = Matrix();
        textState_.tm = Matrix();
    } else if (op == "ET") {
        inTextObject_ = false;
    } else if (op == "Tf") {
        if (operandStack_.size() >= 2) {
            textState_.fontSize = PopNumber();
            textState_.fontId = PopString();
            LoadFontCMap(textState_.fontId);
        }
    } else if (op == "Td") {
        if (operandStack_.size() >= 2) {
            double ty = PopNumber();
            double tx = PopNumber();
            Matrix trans;
            trans.e = tx; trans.f = ty;
            textState_.tlm.Concat(trans);
            textState_.tm = textState_.tlm;
        }
    } else if (op == "Tm") {
        if (operandStack_.size() >= 6) {
            Matrix m;
            m.f = PopNumber();
            m.e = PopNumber();
            m.d = PopNumber();
            m.c = PopNumber();
            m.b = PopNumber();
            m.a = PopNumber();
            textState_.tlm = m;
            textState_.tm = m;
        }
    } else if (op == "Tj") {
        if (operandStack_.size() >= 1) {
            std::string text = PopString();
            extractedText_ += ApplyCMap(text, textState_.fontId);
        }
    } else if (op == "TJ") {
        // Collect everything inside the array backward until '['
        std::string arrayStr = "";
        while (!operandStack_.empty()) {
            auto t = operandStack_.back();
            operandStack_.pop_back();
            if (t.type == parser::TokenType::ArrayStart) {
                break;
            }
            if (t.type == parser::TokenType::StringLiteral || t.type == parser::TokenType::HexString || t.type == parser::TokenType::Name) {
                // Post-fix array reversing prepends string blocks
                arrayStr = t.stringValue + arrayStr;
            }
        }
        // In PDF arrays of TJ, numbers indicate kerning displacement. 
        extractedText_ += ApplyCMap(arrayStr, textState_.fontId);
    } else if (op == "'" || op == "\"") {
        if (operandStack_.size() >= 1) {
            std::string text = PopString();
            extractedText_ += "\n";
            extractedText_ += ApplyCMap(text, textState_.fontId);
        }
    } else if (op == "Do") {
        if (operandStack_.size() >= 1) {
            std::string xobjectName = PopString();
            // Looks up explicit name into Resources->XObject dictionary natively passing extracted bytes to nxgfx without duplicating
        }
    }
}

void ContentInterpreter::LoadFontCMap(const std::string& fontId) {
    if (!resources_ || !xref_) return;
    if (cmaps_.find(fontId) != cmaps_.end()) return; // Already cached
    
    // Explicit reference tracker
    auto Resolve = [&](parser::PdfObject* obj) -> parser::PdfObject* {
        if (auto ref = dynamic_cast<parser::PdfReference*>(obj)) {
            return xref_->GetObject(ref->objectNumber, ref->generationNumber);
        }
        return obj;
    };

    auto fontDictNode = Resolve(resources_->map["Font"].get());
    if (auto fontMap = dynamic_cast<parser::PdfDictionary*>(fontDictNode)) {
        auto fontNode = Resolve(fontMap->map[fontId].get());
        if (auto font = dynamic_cast<parser::PdfDictionary*>(fontNode)) {
            auto toUnicodeNode = Resolve(font->map["ToUnicode"].get());
            if (auto stream = dynamic_cast<parser::PdfStream*>(toUnicodeNode)) {
                // Decode Stream
                bool isFlate = false;
                if (auto filter = dynamic_cast<parser::PdfName*>(stream->dictionary->map["Filter"].get())) {
                    if (filter->value == "FlateDecode") isFlate = true;
                } else if (auto filterArr = dynamic_cast<parser::PdfArray*>(stream->dictionary->map["Filter"].get())) {
                    for (const auto& item : filterArr->items) {
                        if (auto n = dynamic_cast<parser::PdfName*>(item.get())) {
                            if (n->value == "FlateDecode") isFlate = true;
                        }
                    }
                }
                
                std::vector<uint8_t> plainData;
                if (isFlate) {
                    plainData = parser::DecodeFlateData(stream->data);
                } else {
                    plainData = stream->data;
                }
                
                cmaps_[fontId] = std::make_unique<fonts::ToUnicodeCMap>(plainData);
            } else {
                printf("[nxpdf] Font %s does NOT have /ToUnicode\n", fontId.c_str());
            }

            // Always check Encoding
            if (font->map.count("Encoding")) {
                auto encNode = Resolve(font->map["Encoding"].get());
                if (auto encName = dynamic_cast<parser::PdfName*>(encNode)) {
                    fontEncodings_[fontId] = encName->value;
                    printf("[nxpdf] Font %s relies on predefined Encoding: %s\n", fontId.c_str(), encName->value.c_str());
                } else if (auto encDict = dynamic_cast<parser::PdfDictionary*>(encNode)) {
                    printf("[nxpdf] Font %s relies on a custom /Encoding dictionary\n", fontId.c_str());
                    if (encDict->map.count("Differences")) {
                        printf("[nxpdf]   -> Contains /Differences array\n");
                    }
                    if (encDict->map.count("BaseEncoding")) {
                         auto baseEnc = dynamic_cast<parser::PdfName*>(encDict->map["BaseEncoding"].get());
                         if (baseEnc) printf("[nxpdf]   -> BaseEncoding: %s\n", baseEnc->value.c_str());
                    }
                }
            }
        }
    }
}

std::string ContentInterpreter::ApplyCMap(const std::string& rawText, const std::string& fontId) {
    if (cmaps_.count(fontId) && cmaps_[fontId]) {
        std::string result;
        auto* cmap = cmaps_[fontId].get();
        
        // Simple heuristic for 2-byte mappings frequent in IRCTC/Type0 fonts
        bool isTwoByte = cmap->IsTwoByte();
        if (rawText.size() >= 2 && rawText[0] == '\0') {
            isTwoByte = true;
        }

        if (isTwoByte) {
            for (size_t i = 0; i + 1 < rawText.size(); i += 2) {
                int code = (static_cast<uint8_t>(rawText[i]) << 8) | static_cast<uint8_t>(rawText[i+1]);
                result += cmap->MapCodeToUnicode(code);
            }
        } else {
            for (size_t i = 0; i < rawText.size(); i++) {
                int code = static_cast<uint8_t>(rawText[i]);
                result += cmap->MapCodeToUnicode(code);
            }
        }
        return result;
    }
    
    // Valid Latin-1 implicit literal fallback
    std::string cleanText;
    std::string enc = "StandardEncoding";
    if (fontEncodings_.count(fontId)) enc = fontEncodings_[fontId];

    static const char* winAnsiMap[128] = {
        "\xE2\x82\xAC", "", "\xE2\x80\x9A", "\xC6\x92", "\xE2\x80\x9E", "\xE2\x80\xA6", "\xE2\x80\xA0", "\xE2\x80\xA1", 
        "\xCB\x86", "\xE2\x80\xB0", "\xC5\xA0", "\xE2\x80\xB9", "\xC5\x92", "", "\xC5\xBD", "",
        "", "\xE2\x80\x98", "\xE2\x80\x99", "\xE2\x80\x9C", "\xE2\x80\x9D", "\xE2\x80\xA2", "\xE2\x80\x93", "\xE2\x80\x94", 
        "\xCB\x9C", "\xE2\x84\xA2", "\xC5\xA1", "\xE2\x80\xBA", "\xC5\x93", "", "\xC5\xBE", "\xC5\xB8",
        "\xC2\xA0", "\xC2\xA1", "\xC2\xA2", "\xC2\xA3", "\xC2\xA4", "\xC2\xA5", "\xC2\xA6", "\xC2\xA7", 
        "\xC2\xA8", "\xC2\xA9", "\xC2\xAA", "\xC2\xAB", "\xC2\xAC", "\xC2\xAD", "\xC2\xAE", "\xC2\xAF",
        "\xC2\xB0", "\xC2\xB1", "\xC2\xB2", "\xC2\xB3", "\xC2\xB4", "\xC2\xB5", "\xC2\xB6", "\xC2\xB7", 
        "\xC2\xB8", "\xC2\xB9", "\xC2\xBA", "\xC2\xBB", "\xC2\xBC", "\xC2\xBD", "\xC2\xBE", "\xC2\xBF",
        "\xC3\x80", "\xC3\x81", "\xC3\x82", "\xC3\x83", "\xC3\x84", "\xC3\x85", "\xC3\x86", "\xC3\x87", 
        "\xC3\x88", "\xC3\x89", "\xC3\x8A", "\xC3\x8B", "\xC3\x8C", "\xC3\x8D", "\xC3\x8E", "\xC3\x8F",
        "\xC3\x90", "\xC3\x91", "\xC3\x92", "\xC3\x93", "\xC3\x94", "\xC3\x95", "\xC3\x96", "\xC3\x97", 
        "\xC3\x98", "\xC3\x99", "\xC3\x9A", "\xC3\x9B", "\xC3\x9C", "\xC3\x9D", "\xC3\x9E", "\xC3\x9F",
        "\xC3\xA0", "\xC3\xA1", "\xC3\xA2", "\xC3\xA3", "\xC3\xA4", "\xC3\xA5", "\xC3\xA6", "\xC3\xA7", 
        "\xC3\xA8", "\xC3\xA9", "\xC3\xAA", "\xC3\xAB", "\xC3\xAC", "\xC3\xAD", "\xC3\xAE", "\xC3\xAF",
        "\xC3\xB0", "\xC3\xB1", "\xC3\xB2", "\xC3\xB3", "\xC3\xB4", "\xC3\xB5", "\xC3\xB6", "\xC3\xB7", 
        "\xC3\xB8", "\xC3\xB9", "\xC3\xBA", "\xC3\xBB", "\xC3\xBC", "\xC3\xBD", "\xC3\xBE", "\xC3\xBF"
    };

    for (char c : rawText) { 
        if (c == '\0') continue;
        uint8_t u = static_cast<uint8_t>(c);
        if (u < 128) {
            cleanText += c;
        } else {
            if (enc == "WinAnsiEncoding") {
                cleanText += winAnsiMap[u - 128];
            } else {
                // Latin-1 fallback literal mapping limit bounds securely isolating mappings statically
                cleanText += static_cast<char>(0xC0 | (u >> 6));
                cleanText += static_cast<char>(0x80 | (u & 0x3F));
            }
        }
    }
    return cleanText;
}

} // namespace renderer
} // namespace pdf
} // namespace nxrender
