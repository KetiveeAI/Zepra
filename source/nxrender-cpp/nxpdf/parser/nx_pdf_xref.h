/**
 * @file nx_pdf_xref.h
 * @brief XRef table implementation and memory owner mapping
 */

#pragma once

#include "nx_pdf_objects.h"
#include <unordered_map>
#include <memory>

namespace nxrender {
namespace pdf {
namespace parser {

class PdfDocumentParser; // Forward declaration for active parsing callback

enum class XRefEntryType {
    Free,       // 'f'
    InUse,      // 'n'
    Compressed  // PDF 1.5+ compressed streams
};

struct XRefEntry {
    size_t byteOffset = 0;
    int generation = 0;
    XRefEntryType type = XRefEntryType::Free;
    
    // Lazy resolution caching mechanics
    bool resolved = false;
    std::unique_ptr<PdfObject> object = nullptr;
};

class XRefTable {
public:
    explicit XRefTable(PdfDocumentParser* parser);
    ~XRefTable() = default;

    // Registers an unparsed byte offset to an object number
    void RegisterObject(int objectNumber, int generation, size_t offset, XRefEntryType type);

    // Resolves and returns the object pointer natively. 
    // Triggers lazy-load parsing loop if `resolved == false`.
    PdfObject* GetObject(int objectNumber, int generation);
    
    size_t GetCount() const { return table_.size(); }

private:
    PdfDocumentParser* documentParser_;
    std::unordered_map<int, XRefEntry> table_;
};

} // namespace parser
} // namespace pdf
} // namespace nxrender
