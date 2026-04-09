// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "nx_pdf.h"
#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>

namespace nxrender {
namespace pdf {
namespace editor {

// ==================================================================
// Object modification tracking
// ==================================================================

enum class EditOp {
    Modify,
    Insert,
    Delete
};

struct EditRecord {
    EditOp op;
    uint32_t objectId;
    uint32_t generation;
    std::string originalData;
    std::string modifiedData;
};

// ==================================================================
// Document editor
// ==================================================================

class DocumentEditor {
public:
    DocumentEditor() = default;

    // Track object modifications for incremental update
    void recordModification(uint32_t objId, uint32_t gen,
                             const std::string& original,
                             const std::string& modified) {
        EditRecord rec;
        rec.op = EditOp::Modify;
        rec.objectId = objId;
        rec.generation = gen;
        rec.originalData = original;
        rec.modifiedData = modified;
        edits_.push_back(rec);
        modifiedObjects_.insert(objId);
    }

    void recordInsertion(uint32_t objId, uint32_t gen,
                          const std::string& data) {
        EditRecord rec;
        rec.op = EditOp::Insert;
        rec.objectId = objId;
        rec.generation = gen;
        rec.modifiedData = data;
        edits_.push_back(rec);
        modifiedObjects_.insert(objId);
    }

    void recordDeletion(uint32_t objId, uint32_t gen) {
        EditRecord rec;
        rec.op = EditOp::Delete;
        rec.objectId = objId;
        rec.generation = gen + 1; // Increment generation on delete
        edits_.push_back(rec);
        deletedObjects_.insert(objId);
        modifiedObjects_.erase(objId);
    }

    // Undo support
    bool canUndo() const { return !edits_.empty() && undoIndex_ > 0; }
    bool canRedo() const { return undoIndex_ < edits_.size(); }

    void undo() {
        if (!canUndo()) return;
        undoIndex_--;
        // Reverse the edit at undoIndex_
    }

    void redo() {
        if (!canRedo()) return;
        undoIndex_++;
        // Re-apply the edit at undoIndex_-1
    }

    // Page operations
    void deletePage(int pageIndex) {
        deletedPages_.push_back(pageIndex);
    }

    void insertBlankPage(int afterIndex, double width, double height) {
        InsertedPage page;
        page.afterIndex = afterIndex;
        page.width = width;
        page.height = height;
        insertedPages_.push_back(page);
    }

    void rotatePage(int pageIndex, int degrees) {
        pageRotations_[pageIndex] = degrees;
    }

    // Query state
    bool isModified() const {
        return !edits_.empty() || !deletedPages_.empty() ||
               !insertedPages_.empty() || !pageRotations_.empty();
    }

    size_t editCount() const { return edits_.size(); }
    const std::vector<EditRecord>& edits() const { return edits_; }

    bool isObjectModified(uint32_t objId) const {
        return modifiedObjects_.count(objId) > 0;
    }

    bool isObjectDeleted(uint32_t objId) const {
        return deletedObjects_.count(objId) > 0;
    }

    void clear() {
        edits_.clear();
        modifiedObjects_.clear();
        deletedObjects_.clear();
        deletedPages_.clear();
        insertedPages_.clear();
        pageRotations_.clear();
        undoIndex_ = 0;
    }

private:
    std::vector<EditRecord> edits_;
    std::unordered_set<uint32_t> modifiedObjects_;
    std::unordered_set<uint32_t> deletedObjects_;
    size_t undoIndex_ = 0;

    std::vector<int> deletedPages_;

    struct InsertedPage {
        int afterIndex;
        double width, height;
    };
    std::vector<InsertedPage> insertedPages_;

    std::unordered_map<int, int> pageRotations_;
};

} // namespace editor
} // namespace pdf
} // namespace nxrender
