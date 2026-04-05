/**
 * @file nx_pdf.h
 * @brief Public API surface for NXRender PDF capabilities
 */

#pragma once

#include "nx_pdf_backend.h"
#include <string>
#include <memory>

namespace nxrender {
namespace pdf {

class Document {
public:
    virtual ~Document() = default;

    static std::unique_ptr<Document> Open(const std::string& filepath);
    static std::unique_ptr<Document> OpenFromMemory(const std::string& data);
    
    virtual int GetPageCount() const = 0;
    virtual void RenderPage(int pageIndex) = 0;
    virtual std::string ExtractText(int pageIndex) = 0;
};

} // namespace pdf
} // namespace nxrender
