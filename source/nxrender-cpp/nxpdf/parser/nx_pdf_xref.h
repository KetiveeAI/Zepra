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

    void RegisterObject(int objectNumber, int generation, size_t offset, XRefEntryType type);
    PdfObject* GetObject(int objectNumber, int generation);

    // Queries
    bool HasObject(int objectNumber) const;
    int GetGeneration(int objectNumber) const;
    size_t GetOffset(int objectNumber) const;
    bool IsResolved(int objectNumber) const;

    // Iteration
    std::vector<int> GetAllObjectNumbers() const;
    int GetMaxObjectNumber() const;

    // Free list
    void MarkFree(int objectNumber, int nextFreeObj = 0);

    // Bulk operations
    void ResolveAll();

    // Statistics
    size_t GetCount() const { return table_.size(); }
    size_t GetResolvedCount() const;
    size_t GetFreeCount() const;

private:
    PdfDocumentParser* documentParser_;
    std::unordered_map<int, XRefEntry> table_;
};

} // namespace parser
} // namespace pdf
} // namespace nxrender
