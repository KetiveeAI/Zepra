/**
 * @file nx_pdf_xref.cpp
 * @brief XRef table and object offset map
 */

#include "nx_pdf_xref.h"
#include "nx_pdf_document.h"

namespace nxrender {
namespace pdf {
namespace parser {

XRefTable::XRefTable(PdfDocumentParser* parser) : documentParser_(parser) {}

void XRefTable::RegisterObject(int objectNumber, int generation, size_t offset, XRefEntryType type) {
    XRefEntry entry;
    entry.byteOffset = offset;
    entry.generation = generation;
    entry.type = type;
    entry.resolved = false;
    entry.object = nullptr;
    
    table_[objectNumber] = std::move(entry);
}

PdfObject* XRefTable::GetObject(int objectNumber, int generation) {
    auto it = table_.find(objectNumber);
    if (it == table_.end()) {
        return nullptr; // Missing object reference
    }
    
    XRefEntry& entry = it->second;
    
    // Hard unsupported abort for stream objects per Phase 3 definition
    if (entry.type == XRefEntryType::Compressed) {
        return nullptr; 
    }
    
    // Execute Lazy Load
    if (!entry.resolved && entry.type == XRefEntryType::InUse) {
        entry.object = documentParser_->ParseObjectAtOffset(entry.byteOffset);
        entry.resolved = true;
    }
    
    return entry.object.get();
}

} // namespace parser
} // namespace pdf
} // namespace nxrender
