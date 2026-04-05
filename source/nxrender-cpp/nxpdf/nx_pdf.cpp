/**
 * @file nx_pdf.cpp
 * @brief Top-Level abstraction executing API endpoints
 */

#include "nx_pdf.h"
#include "parser/nx_pdf_document.h"
#include "renderer/nx_pdf_page.h"
#include <fstream>
#include <sstream>
#include <iostream>

namespace nxrender {
namespace pdf {

class BuiltinDocument : public Document {
public:
    explicit BuiltinDocument(std::string buffer) 
        : fileBuffer_(std::move(buffer)), parser_(fileBuffer_) {
        // Execute structural tail bindings
        auto err = parser_.Parse();
        if (err != parser::PdfErrorCode::None) {
            std::cerr << "[Debug] parser_.Parse() failed with error code: " << static_cast<int>(err) << std::endl;
        }
    }
    
    int GetPageCount() const override {
        if (auto trailer = parser_.GetTrailer()) {
            if (auto rootRef = dynamic_cast<parser::PdfReference*>(trailer->map["Root"].get())) {
                if (auto catalog = dynamic_cast<parser::PdfDictionary*>(parser_.GetXRefTable().GetObject(rootRef->objectNumber, rootRef->generationNumber))) {
                    if (catalog->map.count("Pages")) {
                        if (auto pagesRef = dynamic_cast<parser::PdfReference*>(catalog->map["Pages"].get())) {
                            if (auto pages = dynamic_cast<parser::PdfDictionary*>(parser_.GetXRefTable().GetObject(pagesRef->objectNumber, pagesRef->generationNumber))) {
                                if (pages->map.count("Count")) {
                                    if (auto count = dynamic_cast<parser::PdfInteger*>(pages->map["Count"].get())) {
                                        return count->value;
                                    } else std::cerr << "[Debug] /Count is not an integer." << std::endl;
                                } else std::cerr << "[Debug] /Pages has no /Count." << std::endl;
                            } else std::cerr << "[Debug] Could not get /Pages object " << pagesRef->objectNumber << std::endl;
                        } else std::cerr << "[Debug] /Pages is not a reference." << std::endl;
                    } else std::cerr << "[Debug] Catalog has no /Pages." << std::endl;
                } else std::cerr << "[Debug] Could not get Catalog object " << rootRef->objectNumber << std::endl;
            } else {
                std::cerr << "[Debug] Trailer /Root is not a reference. It is type: " << (int)trailer->map["Root"]->GetType() << std::endl;
            }
        } else std::cerr << "[Debug] No trailer found." << std::endl;
        return 0; // Malformed boundary fallback cleanly
    }

    void RenderPage(int index) override {
        // Recursion mapping logic identifying specific layout index bounds natively deferred for explicit rendering isolation
    }

    std::string ExtractText(int index) override {
        // Find Page Dict
        if (auto trailer = parser_.GetTrailer()) {
            if (auto rootRef = dynamic_cast<parser::PdfReference*>(trailer->map["Root"].get())) {
                if (auto catalog = dynamic_cast<parser::PdfDictionary*>(parser_.GetXRefTable().GetObject(rootRef->objectNumber, rootRef->generationNumber))) {
                    if (catalog->map.count("Pages")) {
                        if (auto pagesRef = dynamic_cast<parser::PdfReference*>(catalog->map["Pages"].get())) {
                            if (auto pages = dynamic_cast<parser::PdfDictionary*>(parser_.GetXRefTable().GetObject(pagesRef->objectNumber, pagesRef->generationNumber))) {
                                // Extract Kids -> Find explicit mapping index (Iterating simple 0-n bound directly for MVP simple loop extraction)
                                if (pages->map.count("Kids")) {
                                    if (auto kids = dynamic_cast<parser::PdfArray*>(pages->map["Kids"].get())) {
                                        if (index >= 0 && index < kids->items.size()) {
                                            if (auto pageRef = dynamic_cast<parser::PdfReference*>(kids->items[index].get())) {
                                                if (auto pageDict = dynamic_cast<parser::PdfDictionary*>(parser_.GetXRefTable().GetObject(pageRef->objectNumber, pageRef->generationNumber))) {
                                                    renderer::PdfPage page(pageDict, &(parser_.GetXRefTable()));
                                                    return page.ExtractText();
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        return "";
    }

private:
    std::string fileBuffer_;
    mutable parser::PdfDocumentParser parser_;
};

std::unique_ptr<Document> Document::Open(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) return nullptr;
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    
    // Explicit static memory copying ensures lifetime constraints decouple precisely natively 
    return std::make_unique<BuiltinDocument>(buffer.str());
}

std::unique_ptr<Document> Document::OpenFromMemory(const std::string& data) {
    return std::make_unique<BuiltinDocument>(data);
}

} // namespace pdf
} // namespace nxrender
