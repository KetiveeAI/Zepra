/**
 * @file source_code.cpp
 * @brief Source file implementation
 */

#include "frontend/source_code.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace Zepra::Frontend {

SourceCode::SourceCode(std::string content, std::string filename)
    : content_(std::move(content))
    , filename_(std::move(filename))
{
    buildLineIndex();
}

std::unique_ptr<SourceCode> SourceCode::fromString(std::string source,
                                                    std::string filename) {
    return std::unique_ptr<SourceCode>(
        new SourceCode(std::move(source), std::move(filename)));
}

std::unique_ptr<SourceCode> SourceCode::fromFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return nullptr;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    
    return fromString(buffer.str(), filepath);
}

char SourceCode::at(size_t offset) const {
    if (offset >= content_.length()) {
        return '\0';
    }
    return content_[offset];
}

std::string_view SourceCode::substring(size_t start, size_t length) const {
    if (start >= content_.length()) {
        return {};
    }
    return std::string_view(content_).substr(start, length);
}

std::string_view SourceCode::getLine(uint32_t lineNumber) const {
    if (lineNumber == 0 || lineNumber > lineOffsets_.size()) {
        return {};
    }
    
    size_t start = lineOffsets_[lineNumber - 1];
    size_t end;
    
    if (lineNumber < lineOffsets_.size()) {
        end = lineOffsets_[lineNumber];
        // Remove trailing newline
        if (end > start && content_[end - 1] == '\n') {
            end--;
        }
        if (end > start && content_[end - 1] == '\r') {
            end--;
        }
    } else {
        end = content_.length();
    }
    
    return std::string_view(content_).substr(start, end - start);
}

uint32_t SourceCode::lineAt(size_t offset) const {
    auto it = std::upper_bound(lineOffsets_.begin(), lineOffsets_.end(), offset);
    return static_cast<uint32_t>(it - lineOffsets_.begin());
}

uint32_t SourceCode::columnAt(size_t offset) const {
    uint32_t line = lineAt(offset);
    if (line == 0) return 1;
    
    size_t lineStart = lineOffsets_[line - 1];
    return static_cast<uint32_t>(offset - lineStart + 1);
}

uint32_t SourceCode::lineCount() const {
    return static_cast<uint32_t>(lineOffsets_.size());
}

void SourceCode::buildLineIndex() {
    lineOffsets_.clear();
    lineOffsets_.push_back(0);  // First line starts at offset 0
    
    for (size_t i = 0; i < content_.length(); i++) {
        if (content_[i] == '\n') {
            lineOffsets_.push_back(i + 1);
        }
    }
}

} // namespace Zepra::Frontend
