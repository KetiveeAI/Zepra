/**
 * @file ReleaseManifest.h
 * @brief Release Engineering and Manifest
 * 
 * Implements:
 * - Version tracking
 * - Changelog management
 * - Known limitations
 * - Upgrade path validation
 */

#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace Zepra::Release {

// =============================================================================
// Changelog Entry
// =============================================================================

enum class ChangeType {
    Feature,
    BugFix,
    Security,
    Performance,
    Breaking,
    Deprecated
};

struct ChangelogEntry {
    ChangeType type;
    std::string description;
    std::string issueRef;  // e.g., "ZEPRA-123"
};

// =============================================================================
// Release Manifest
// =============================================================================

/**
 * @brief Complete release information
 */
class ReleaseManifest {
public:
    // Version
    uint32_t major = 1;
    uint32_t minor = 0;
    uint32_t patch = 0;
    std::string preRelease;  // e.g., "beta.1"
    
    // Build info
    std::string buildId;
    std::string gitCommit;
    std::string buildDate;
    
    // Changelog
    std::vector<ChangelogEntry> changes;
    
    // Known limitations
    std::vector<std::string> knownLimitations;
    
    // Compatibility
    uint32_t minUpgradeFromMajor = 0;
    uint32_t minUpgradeFromMinor = 0;
    
    // Version string
    std::string versionString() const {
        std::string v = std::to_string(major) + "." +
                       std::to_string(minor) + "." +
                       std::to_string(patch);
        if (!preRelease.empty()) {
            v += "-" + preRelease;
        }
        return v;
    }
    
    // Full identifier
    std::string fullIdentifier() const {
        return "ZepraScript v" + versionString() + 
               " (" + gitCommit.substr(0, 8) + ")";
    }
    
    // Check upgrade compatibility
    bool canUpgradeFrom(uint32_t fromMajor, uint32_t fromMinor) const {
        if (fromMajor < minUpgradeFromMajor) return false;
        if (fromMajor == minUpgradeFromMajor && 
            fromMinor < minUpgradeFromMinor) return false;
        return true;
    }
    
    // Generate changelog text
    std::string changelogText() const {
        std::string text = "# Changelog for " + versionString() + "\n\n";
        
        // Group by type
        std::vector<std::pair<ChangeType, std::string>> groups = {
            {ChangeType::Breaking, "⚠️ Breaking Changes"},
            {ChangeType::Security, "🔒 Security"},
            {ChangeType::Feature, "✨ Features"},
            {ChangeType::BugFix, "🐛 Bug Fixes"},
            {ChangeType::Performance, "⚡ Performance"},
            {ChangeType::Deprecated, "🗑️ Deprecated"}
        };
        
        for (const auto& [type, header] : groups) {
            std::vector<const ChangelogEntry*> entries;
            for (const auto& e : changes) {
                if (e.type == type) entries.push_back(&e);
            }
            
            if (!entries.empty()) {
                text += "## " + header + "\n\n";
                for (const auto* e : entries) {
                    text += "- " + e->description;
                    if (!e->issueRef.empty()) {
                        text += " (" + e->issueRef + ")";
                    }
                    text += "\n";
                }
                text += "\n";
            }
        }
        
        // Known limitations
        if (!knownLimitations.empty()) {
            text += "## Known Limitations\n\n";
            for (const auto& lim : knownLimitations) {
                text += "- " + lim + "\n";
            }
        }
        
        return text;
    }
};

// =============================================================================
// Release Builder
// =============================================================================

/**
 * @brief Builds release manifest
 */
class ReleaseBuilder {
public:
    ReleaseBuilder& version(uint32_t maj, uint32_t min, uint32_t pat) {
        manifest_.major = maj;
        manifest_.minor = min;
        manifest_.patch = pat;
        return *this;
    }
    
    ReleaseBuilder& preRelease(const std::string& pre) {
        manifest_.preRelease = pre;
        return *this;
    }
    
    ReleaseBuilder& build(const std::string& id, const std::string& commit) {
        manifest_.buildId = id;
        manifest_.gitCommit = commit;
        return *this;
    }
    
    ReleaseBuilder& feature(const std::string& desc, const std::string& ref = "") {
        manifest_.changes.push_back({ChangeType::Feature, desc, ref});
        return *this;
    }
    
    ReleaseBuilder& bugfix(const std::string& desc, const std::string& ref = "") {
        manifest_.changes.push_back({ChangeType::BugFix, desc, ref});
        return *this;
    }
    
    ReleaseBuilder& security(const std::string& desc, const std::string& ref = "") {
        manifest_.changes.push_back({ChangeType::Security, desc, ref});
        return *this;
    }
    
    ReleaseBuilder& breaking(const std::string& desc, const std::string& ref = "") {
        manifest_.changes.push_back({ChangeType::Breaking, desc, ref});
        return *this;
    }
    
    ReleaseBuilder& limitation(const std::string& lim) {
        manifest_.knownLimitations.push_back(lim);
        return *this;
    }
    
    ReleaseBuilder& upgradeFrom(uint32_t maj, uint32_t min) {
        manifest_.minUpgradeFromMajor = maj;
        manifest_.minUpgradeFromMinor = min;
        return *this;
    }
    
    ReleaseManifest build() {
        return std::move(manifest_);
    }
    
private:
    ReleaseManifest manifest_;
};

// =============================================================================
// Example Release
// =============================================================================

inline ReleaseManifest createV1Release() {
    return ReleaseBuilder()
        .version(1, 0, 0)
        .build("001", "abc123def456")
        .feature("Complete ES2024 support")
        .feature("WASM Component Model")
        .feature("Multi-tier JIT compilation")
        .feature("Generational garbage collection")
        .security("Memory safety enforcement")
        .security("Cross-realm isolation")
        .limitation("No asm.js support")
        .limitation("No SharedArrayBuffer on non-HTTPS")
        .upgradeFrom(0, 9)
        .build();
}

} // namespace Zepra::Release
