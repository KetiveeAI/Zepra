/**
 * @file nx_pdf_page.h
 * @brief Represents an explicitly bounded page executing graphics states
 */

#pragma once

#include "../parser/nx_pdf_objects.h"
#include "../parser/nx_pdf_document.h"
#include <memory>

namespace nxrender {
namespace pdf {
namespace renderer {

class PdfPage {
public:
    explicit PdfPage(parser::PdfDictionary* pageDict, parser::XRefTable* xref);
    ~PdfPage() = default;

    // Binds explicitly to the native stream
    void Render();
    std::string ExtractText();

private:
    parser::PdfDictionary* pageDict_;
    parser::XRefTable* xref_;
};

} // namespace renderer
} // namespace pdf
} // namespace nxrender
