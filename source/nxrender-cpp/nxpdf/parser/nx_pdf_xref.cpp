// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "nx_pdf_xref.h"
#include "nx_pdf_document.h"
#include <algorithm>
#include <cstring>

namespace nxrender {
namespace pdf {
namespace parser {

XRefTable::XRefTable(PdfDocumentParser* parser) : documentParser_(parser) {}

// ==================================================================
// Registration
// ==================================================================

void XRefTable::RegisterObject(int objectNumber, int generation,
                                size_t offset, XRefEntryType type) {
    // Only register if not already present (first definition wins per PDF spec)
    auto it = table_.find(objectNumber);
    if (it != table_.end()) {
        // Higher generation supersedes
        if (generation <= it->second.generation) return;
    }

    XRefEntry entry;
    entry.byteOffset = offset;
    entry.generation = generation;
    entry.type = type;
    entry.resolved = false;
    entry.object = nullptr;

    table_[objectNumber] = std::move(entry);
}

// ==================================================================
// Object resolution
// ==================================================================

PdfObject* XRefTable::GetObject(int objectNumber, int generation) {
    auto it = table_.find(objectNumber);
    if (it == table_.end()) return nullptr;

    XRefEntry& entry = it->second;

    // Free entries yield nothing
    if (entry.type == XRefEntryType::Free) return nullptr;

    // Compressed stream objects not yet supported
    if (entry.type == XRefEntryType::Compressed) return nullptr;

    // Lazy load
    if (!entry.resolved && entry.type == XRefEntryType::InUse) {
        if (documentParser_) {
            entry.object = documentParser_->ParseObjectAtOffset(entry.byteOffset);
        }
        entry.resolved = true;
    }

    return entry.object.get();
}

// ==================================================================
// Queries
// ==================================================================

bool XRefTable::HasObject(int objectNumber) const {
    auto it = table_.find(objectNumber);
    if (it == table_.end()) return false;
    return it->second.type != XRefEntryType::Free;
}

int XRefTable::GetGeneration(int objectNumber) const {
    auto it = table_.find(objectNumber);
    if (it == table_.end()) return -1;
    return it->second.generation;
}

size_t XRefTable::GetOffset(int objectNumber) const {
    auto it = table_.find(objectNumber);
    if (it == table_.end()) return 0;
    return it->second.byteOffset;
}

bool XRefTable::IsResolved(int objectNumber) const {
    auto it = table_.find(objectNumber);
    if (it == table_.end()) return false;
    return it->second.resolved;
}

// ==================================================================
// Iteration
// ==================================================================

std::vector<int> XRefTable::GetAllObjectNumbers() const {
    std::vector<int> result;
    result.reserve(table_.size());
    for (const auto& pair : table_) {
        if (pair.second.type == XRefEntryType::InUse) {
            result.push_back(pair.first);
        }
    }
    std::sort(result.begin(), result.end());
    return result;
}

int XRefTable::GetMaxObjectNumber() const {
    int maxNum = 0;
    for (const auto& pair : table_) {
        maxNum = std::max(maxNum, pair.first);
    }
    return maxNum;
}

// ==================================================================
// Free list management
// ==================================================================

void XRefTable::MarkFree(int objectNumber, int nextFreeObj) {
    auto it = table_.find(objectNumber);
    if (it != table_.end()) {
        it->second.type = XRefEntryType::Free;
        it->second.resolved = false;
        it->second.object.reset();
        it->second.generation++;
    } else {
        XRefEntry entry;
        entry.type = XRefEntryType::Free;
        entry.byteOffset = nextFreeObj; // Free list uses offset as next free ptr
        entry.generation = 0;
        table_[objectNumber] = std::move(entry);
    }
}

void XRefTable::ResolveAll() {
    for (auto& pair : table_) {
        if (!pair.second.resolved && pair.second.type == XRefEntryType::InUse) {
            GetObject(pair.first, pair.second.generation);
        }
    }
}

size_t XRefTable::GetResolvedCount() const {
    size_t count = 0;
    for (const auto& pair : table_) {
        if (pair.second.resolved) count++;
    }
    return count;
}

size_t XRefTable::GetFreeCount() const {
    size_t count = 0;
    for (const auto& pair : table_) {
        if (pair.second.type == XRefEntryType::Free) count++;
    }
    return count;
}

} // namespace parser
} // namespace pdf
} // namespace nxrender
