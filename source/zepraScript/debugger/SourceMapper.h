/**
 * @file SourceMapper.h
 * @brief Source mapping for minified/transpiled code
 * 
 * Implements:
 * - Source map v3 parsing
 * - Original location lookup
 * - Generated location lookup
 * - Inline source maps
 * 
 * Based on Mozilla source-map library
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <memory>

namespace Zepra::Debug {

// =============================================================================
// Source Map Mapping
// =============================================================================

struct MappingEntry {
    // Generated (output) location
    uint32_t generatedLine = 0;
    uint32_t generatedColumn = 0;
    
    // Original (source) location
    uint32_t sourceIndex = 0;
    uint32_t originalLine = 0;
    uint32_t originalColumn = 0;
    
    // Optional name index
    std::optional<uint32_t> nameIndex;
};

// =============================================================================
// Source Map
// =============================================================================

class SourceMap {
public:
    SourceMap() = default;
    
    /**
     * @brief Parse source map from JSON string
     */
    static std::unique_ptr<SourceMap> parse(const std::string& json);
    
    /**
     * @brief Parse inline source map (data: URL)
     */
    static std::unique_ptr<SourceMap> parseInline(const std::string& dataUrl);
    
    // =========================================================================
    // Metadata
    // =========================================================================
    
    int version() const { return version_; }
    const std::string& file() const { return file_; }
    const std::string& sourceRoot() const { return sourceRoot_; }
    
    // =========================================================================
    // Sources
    // =========================================================================
    
    size_t sourceCount() const { return sources_.size(); }
    const std::string& getSource(size_t index) const { return sources_[index]; }
    const std::vector<std::string>& sources() const { return sources_; }
    
    bool hasSourceContent(size_t index) const;
    const std::string& getSourceContent(size_t index) const;
    
    // =========================================================================
    // Names
    // =========================================================================
    
    size_t nameCount() const { return names_.size(); }
    const std::string& getName(size_t index) const { return names_[index]; }
    const std::vector<std::string>& names() const { return names_; }
    
    // =========================================================================
    // Lookups
    // =========================================================================
    
    /**
     * @brief Find original location from generated location
     */
    std::optional<MappingEntry> originalPositionFor(
        uint32_t generatedLine, 
        uint32_t generatedColumn) const;
    
    /**
     * @brief Find generated location from original location
     */
    std::optional<MappingEntry> generatedPositionFor(
        const std::string& source,
        uint32_t originalLine,
        uint32_t originalColumn) const;
    
    /**
     * @brief Get all mappings for a generated line
     */
    std::vector<MappingEntry> mappingsForLine(uint32_t generatedLine) const;
    
    // =========================================================================
    // Iteration
    // =========================================================================
    
    void eachMapping(std::function<void(const MappingEntry&)> callback) const;
    
private:
    int version_ = 3;
    std::string file_;
    std::string sourceRoot_;
    std::vector<std::string> sources_;
    std::vector<std::string> sourcesContent_;
    std::vector<std::string> names_;
    std::string mappings_;
    
    // Parsed mappings by generated line
    std::vector<std::vector<MappingEntry>> parsedMappings_;
    
    // Index for reverse lookup (source:line -> generated)
    std::unordered_map<std::string, std::vector<MappingEntry>> reverseIndex_;
    
    void parseMappings();
    int32_t decodeVLQ(const char*& p);
    void buildReverseIndex();
};

// =============================================================================
// Source Map Registry
// =============================================================================

class SourceMapRegistry {
public:
    SourceMapRegistry() = default;
    
    /**
     * @brief Register source map for a script
     */
    void registerSourceMap(const std::string& scriptUrl, 
                           std::unique_ptr<SourceMap> sourceMap);
    
    /**
     * @brief Get source map for script
     */
    SourceMap* getSourceMap(const std::string& scriptUrl);
    const SourceMap* getSourceMap(const std::string& scriptUrl) const;
    
    /**
     * @brief Remove source map
     */
    void unregisterSourceMap(const std::string& scriptUrl);
    
    /**
     * @brief Check if script has source map
     */
    bool hasSourceMap(const std::string& scriptUrl) const;
    
    /**
     * @brief Map generated location to original
     */
    struct OriginalLocation {
        std::string sourceUrl;
        uint32_t line;
        uint32_t column;
        std::optional<std::string> name;
    };
    
    std::optional<OriginalLocation> mapToOriginal(
        const std::string& scriptUrl,
        uint32_t generatedLine,
        uint32_t generatedColumn) const;
    
    /**
     * @brief Map original location to generated
     */
    struct GeneratedLocation {
        std::string scriptUrl;
        uint32_t line;
        uint32_t column;
    };
    
    std::optional<GeneratedLocation> mapToGenerated(
        const std::string& sourceUrl,
        uint32_t originalLine,
        uint32_t originalColumn) const;
    
private:
    std::unordered_map<std::string, std::unique_ptr<SourceMap>> maps_;
    std::unordered_map<std::string, std::string> sourceToScript_;
};

// =============================================================================
// Source Map Utilities
// =============================================================================

namespace SourceMapUtils {

/**
 * @brief Extract source map URL from script source
 * Looks for //# sourceMappingURL=...
 */
std::optional<std::string> extractSourceMapUrl(const std::string& source);

/**
 * @brief Check if URL is inline data URL
 */
bool isInlineSourceMap(const std::string& url);

/**
 * @brief Decode base64 content from data URL
 */
std::string decodeDataUrl(const std::string& dataUrl);

/**
 * @brief Resolve relative source map URL
 */
std::string resolveUrl(const std::string& baseUrl, const std::string& relativeUrl);

} // namespace SourceMapUtils

} // namespace Zepra::Debug
