#pragma once

/**
 * @file source_code.hpp
 * @brief Source file representation
 */

#include "../config.hpp"
#include <string>
#include <string_view>
#include <vector>
#include <memory>

namespace Zepra::Frontend {

/**
 * @brief Represents a JavaScript source file
 */
class SourceCode {
public:
    /**
     * @brief Create from string content
     */
    static std::unique_ptr<SourceCode> fromString(std::string source,
                                                   std::string filename = "<eval>");
    
    /**
     * @brief Load from file
     */
    static std::unique_ptr<SourceCode> fromFile(const std::string& filepath);
    
    ~SourceCode() = default;
    
    const std::string& content() const { return content_; }
    const std::string& filename() const { return filename_; }
    size_t length() const { return content_.length(); }
    
    /**
     * @brief Get character at offset
     */
    char at(size_t offset) const;
    
    /**
     * @brief Get a substring
     */
    std::string_view substring(size_t start, size_t length) const;
    
    /**
     * @brief Get line at 1-based line number
     */
    std::string_view getLine(uint32_t lineNumber) const;
    
    /**
     * @brief Get line number for offset (1-based)
     */
    uint32_t lineAt(size_t offset) const;
    
    /**
     * @brief Get column for offset (1-based)
     */
    uint32_t columnAt(size_t offset) const;
    
    /**
     * @brief Total number of lines
     */
    uint32_t lineCount() const;
    
private:
    SourceCode(std::string content, std::string filename);
    
    void buildLineIndex();
    
    std::string content_;
    std::string filename_;
    std::vector<size_t> lineOffsets_;  // Offset of each line start
};

} // namespace Zepra::Frontend
