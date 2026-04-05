/**
 * @file nx_pdf_page.cpp
 * @brief Executes explicit page contexts 
 */

#include "nx_pdf_page.h"
#include "nx_pdf_content.h"

#include "../parser/nx_pdf_document.h"

namespace nxrender {
namespace pdf {
namespace renderer {

PdfPage::PdfPage(parser::PdfDictionary* pageDict, parser::XRefTable* xref) 
    : pageDict_(pageDict), xref_(xref) {}

namespace {
    parser::PdfStream* ResolveStream(parser::PdfObject* obj, parser::XRefTable* xref) {
        if (auto stream = dynamic_cast<parser::PdfStream*>(obj)) return stream;
        if (auto ref = dynamic_cast<parser::PdfReference*>(obj)) {
            if (xref) {
                return dynamic_cast<parser::PdfStream*>(xref->GetObject(ref->objectNumber, ref->generationNumber));
            }
        }
        return nullptr;
    }
    
    std::string ProcessStream(parser::PdfStream* stream, parser::PdfDictionary* resources, parser::XRefTable* xref) {
        if (!stream) return "";
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
        
        std::string_view streamView(reinterpret_cast<const char*>(plainData.data()), plainData.size());
        ContentInterpreter interpreter(streamView, resources, xref);
        interpreter.Interpret();
        return interpreter.GetExtractedText();
    }
}

void PdfPage::Render() {
    if (!pageDict_) return;
    auto resources = dynamic_cast<parser::PdfDictionary*>(pageDict_->map["Resources"].get());
    auto contentsIt = pageDict_->map.find("Contents");
    if (contentsIt != pageDict_->map.end()) {
        ProcessStream(ResolveStream(contentsIt->second.get(), xref_), resources, xref_);
    }
}

std::string PdfPage::ExtractText() {
    if (!pageDict_) return "";
    auto resources = dynamic_cast<parser::PdfDictionary*>(pageDict_->map["Resources"].get());
    auto contentsIt = pageDict_->map.find("Contents");
    if (contentsIt != pageDict_->map.end()) {
        return ProcessStream(ResolveStream(contentsIt->second.get(), xref_), resources, xref_);
    }
    return "";
}

} // namespace renderer
} // namespace pdf
} // namespace nxrender
