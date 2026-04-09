// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "nx_pdf.h"
#include <string>
#include <vector>
#include <cstdint>
#include <sstream>
#include <algorithm>
#include <iomanip>

namespace nxrender {
namespace pdf {
namespace editor {

// ==================================================================
// PDF incremental write support
// ==================================================================

struct XRefEntry {
    uint64_t offset;
    uint32_t objectId;
    uint32_t generation;
    bool inUse;    // 'n' for in-use, 'f' for free
};

// ==================================================================
// PDF object serializer
// ==================================================================

class PDFWriter {
public:
    PDFWriter() = default;

    // Start writing at the given offset (end of original file)
    void beginIncrementalUpdate(uint64_t startOffset) {
        startOffset_ = startOffset;
        currentOffset_ = startOffset;
        xrefEntries_.clear();
        output_.str("");
        output_.clear();
    }

    // Write a modified or new indirect object
    void writeObject(uint32_t objId, uint32_t gen, const std::string& content) {
        XRefEntry entry;
        entry.offset = currentOffset_;
        entry.objectId = objId;
        entry.generation = gen;
        entry.inUse = true;
        xrefEntries_.push_back(entry);

        std::string header = std::to_string(objId) + " " +
                             std::to_string(gen) + " obj\n";
        output_ << header;
        currentOffset_ += header.size();

        output_ << content << "\n";
        currentOffset_ += content.size() + 1;

        std::string footer = "endobj\n";
        output_ << footer;
        currentOffset_ += footer.size();
    }

    // Write a free object entry
    void writeFreeObject(uint32_t objId, uint32_t gen) {
        XRefEntry entry;
        entry.offset = 0;
        entry.objectId = objId;
        entry.generation = gen;
        entry.inUse = false;
        xrefEntries_.push_back(entry);
    }

    // Write the cross-reference table and trailer
    std::string finalize(uint32_t prevXRefOffset, uint32_t rootObjId,
                          uint32_t totalObjects) {
        uint64_t xrefOffset = currentOffset_;

        // Sort xref entries by object ID
        std::sort(xrefEntries_.begin(), xrefEntries_.end(),
            [](const XRefEntry& a, const XRefEntry& b) {
                return a.objectId < b.objectId;
            });

        // Write xref table
        output_ << "xref\n";

        // Group contiguous object IDs into subsections
        size_t i = 0;
        while (i < xrefEntries_.size()) {
            uint32_t startId = xrefEntries_[i].objectId;
            size_t count = 1;
            while (i + count < xrefEntries_.size() &&
                   xrefEntries_[i + count].objectId == startId + count) {
                count++;
            }

            output_ << startId << " " << count << "\n";

            for (size_t j = 0; j < count; j++) {
                const auto& entry = xrefEntries_[i + j];
                output_ << std::setfill('0') << std::setw(10)
                        << entry.offset << " "
                        << std::setfill('0') << std::setw(5)
                        << entry.generation << " "
                        << (entry.inUse ? "n" : "f") << " \n";
            }

            i += count;
        }

        // Write trailer
        output_ << "trailer\n";
        output_ << "<<\n";
        output_ << "/Size " << totalObjects << "\n";
        output_ << "/Root " << rootObjId << " 0 R\n";
        output_ << "/Prev " << prevXRefOffset << "\n";
        output_ << ">>\n";
        output_ << "startxref\n";
        output_ << xrefOffset << "\n";
        output_ << "%%EOF\n";

        return output_.str();
    }

    // Get the current output data
    const std::string& data() const {
        static std::string cached;
        cached = output_.str();
        return cached;
    }

    size_t objectCount() const { return xrefEntries_.size(); }

private:
    uint64_t startOffset_ = 0;
    uint64_t currentOffset_ = 0;
    std::vector<XRefEntry> xrefEntries_;
    std::ostringstream output_;
};

} // namespace editor
} // namespace pdf
} // namespace nxrender
