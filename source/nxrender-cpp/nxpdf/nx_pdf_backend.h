/**
 * @file nx_pdf_backend.h
 * @brief Abstraction layer for PDF processing resolving to built-in or OS (NeolyxOS IPC)
 */

#pragma once

#include <memory>

namespace nxrender {
namespace pdf {
namespace backend {

class PdfBackend {
public:
    virtual ~PdfBackend() = default;
    
    virtual void Initialize() = 0;
    virtual void Shutdown() = 0;
};

// Returns the active backend based on compile flags
std::unique_ptr<PdfBackend> CreateActiveBackend();

} // namespace backend
} // namespace pdf
} // namespace nxrender
