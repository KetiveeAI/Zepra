/**
 * @file SourceLocation.h
 * @brief Source location tracking for AST nodes and diagnostics
 * 
 * Implements:
 * - Line/column tracking
 * - Source ranges
 * - Location caching
 * - Source map integration
 * 
 * For better error messages and debugger support
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <optional>

namespace Zepra::Frontend {

// =============================================================================
// Position (line/column)
// =============================================================================

struct Position {
    uint32_t line = 1;      // 1-based
    uint32_t column = 1;    // 1-based
    uint32_t offset = 0;    // 0-based byte offset
    
    bool operator==(const Position& other) const {
        return line == other.line && column == other.column;
    }
    
    bool operator<(const Position& other) const {
        if (line != other.line) return line < other.line;
        return column < other.column;
    }
    
    bool operator<=(const Position& other) const {
        return *this == other || *this < other;
    }
    
    std::string toString() const {
        return std::to_string(line) + ":" + std::to_string(column);
    }
};

// =============================================================================
// Source Range
// =============================================================================

struct SourceRange {
    Position start;
    Position end;
    
    bool isValid() const {
        return start.line > 0 && end.line > 0;
    }
    
    bool contains(const Position& pos) const {
        return start <= pos && pos <= end;
    }
    
    bool overlaps(const SourceRange& other) const {
        return !(end < other.start || other.end < start);
    }
    
    size_t byteLength() const {
        return end.offset - start.offset;
    }
    
    std::string toString() const {
        return start.toString() + "-" + end.toString();
    }
};

// =============================================================================
// Source Location
// =============================================================================

struct SourceLocation {
    std::string filename;
    SourceRange range;
    
    // Optional: original location (for source maps)
    std::optional<std::string> originalFilename;
    std::optional<SourceRange> originalRange;
    
    bool isValid() const {
        return range.isValid();
    }
    
    Position start() const { return range.start; }
    Position end() const { return range.end; }
    
    std::string toString() const {
        return filename + ":" + range.start.toString();
    }
    
    // For source map support
    bool hasOriginalLocation() const {
        return originalFilename.has_value() && originalRange.has_value();
    }
};

// =============================================================================
// Line Info Cache
// =============================================================================

/**
 * @brief Caches line start offsets for fast line/column lookup
 */
class LineInfoCache {
public:
    explicit LineInfoCache(const std::string& source);
    
    /**
     * @brief Get position from byte offset
     */
    Position positionFromOffset(size_t offset) const;
    
    /**
     * @brief Get byte offset from position
     */
    size_t offsetFromPosition(const Position& pos) const;
    
    /**
     * @brief Get line text
     */
    std::string_view getLine(uint32_t lineNumber) const;
    
    /**
     * @brief Get range of source text
     */
    std::string_view getText(const SourceRange& range) const;
    
    /**
     * @brief Total line count
     */
    size_t lineCount() const { return lineStarts_.size(); }
    
private:
    std::string_view source_;
    std::vector<size_t> lineStarts_;
    
    void buildLineIndex();
};

// =============================================================================
// Source File
// =============================================================================

/**
 * @brief Represents a source file
 */
class SourceFile {
public:
    SourceFile(std::string filename, std::string content);
    
    const std::string& filename() const { return filename_; }
    const std::string& content() const { return content_; }
    size_t size() const { return content_.size(); }
    
    // Location operations
    Position positionAt(size_t offset) const;
    size_t offsetAt(const Position& pos) const;
    std::string_view lineAt(uint32_t line) const;
    std::string_view textAt(const SourceRange& range) const;
    
    SourceLocation createLocation(const SourceRange& range) const;
    
private:
    std::string filename_;
    std::string content_;
    LineInfoCache lineInfo_;
};

// =============================================================================
// Diagnostic Location
// =============================================================================

/**
 * @brief Location information for error/warning messages
 */
struct DiagnosticLocation {
    SourceLocation location;
    std::string contextLine;        // The line of source code
    std::string highlightLine;      // "^^^" pointing to error
    
    static DiagnosticLocation create(const SourceFile& file, const SourceRange& range);
    
    std::string format() const;
};

// =============================================================================
// Location Builder
// =============================================================================

/**
 * @brief Helper for building source locations during parsing
 */
class LocationBuilder {
public:
    explicit LocationBuilder(SourceFile* file) : file_(file) {}
    
    void setStart(size_t offset);
    void setEnd(size_t offset);
    
    SourceLocation build() const;
    SourceRange buildRange() const;
    
    void reset() {
        startOffset_ = 0;
        endOffset_ = 0;
    }
    
private:
    SourceFile* file_;
    size_t startOffset_ = 0;
    size_t endOffset_ = 0;
};

} // namespace Zepra::Frontend
