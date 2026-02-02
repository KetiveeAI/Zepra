/**
 * @file FormDataAPI.h
 * @brief FormData API Implementation
 * 
 * XMLHttpRequest Standard:
 * - FormData: Form field handling for fetch/XHR
 */

#pragma once

#include "BlobAPI.h"
#include <string>
#include <vector>
#include <variant>
#include <optional>

namespace Zepra::API {

// =============================================================================
// FormData Entry
// =============================================================================

using FormDataValue = std::variant<std::string, File>;

struct FormDataEntry {
    std::string name;
    FormDataValue value;
};

// =============================================================================
// FormData
// =============================================================================

/**
 * @brief Form field collection for submissions
 */
class FormData {
public:
    FormData() = default;
    
    // From HTML form (would extract fields)
    // explicit FormData(HTMLFormElement* form);
    
    // Append string
    void append(const std::string& name, const std::string& value) {
        entries_.push_back({name, value});
    }
    
    // Append file
    void append(const std::string& name, const File& file) {
        entries_.push_back({name, file});
    }
    
    // Append blob as file
    void append(const std::string& name, const Blob& blob,
                const std::string& filename = "blob") {
        File file(blob.arrayBuffer(), filename, blob.type());
        entries_.push_back({name, file});
    }
    
    // Set (replaces existing)
    void set(const std::string& name, const std::string& value) {
        deleteAll(name);
        append(name, value);
    }
    
    void set(const std::string& name, const File& file) {
        deleteAll(name);
        append(name, file);
    }
    
    // Get first value
    std::optional<FormDataValue> get(const std::string& name) const {
        for (const auto& entry : entries_) {
            if (entry.name == name) {
                return entry.value;
            }
        }
        return std::nullopt;
    }
    
    // Get all values
    std::vector<FormDataValue> getAll(const std::string& name) const {
        std::vector<FormDataValue> result;
        for (const auto& entry : entries_) {
            if (entry.name == name) {
                result.push_back(entry.value);
            }
        }
        return result;
    }
    
    // Has entry
    bool has(const std::string& name) const {
        for (const auto& entry : entries_) {
            if (entry.name == name) return true;
        }
        return false;
    }
    
    // Delete all with name
    void deleteAll(const std::string& name) {
        entries_.erase(
            std::remove_if(entries_.begin(), entries_.end(),
                [&name](const auto& e) { return e.name == name; }),
            entries_.end());
    }
    
    // Iteration
    const std::vector<FormDataEntry>& entries() const { return entries_; }
    
    std::vector<std::string> keys() const {
        std::vector<std::string> result;
        for (const auto& entry : entries_) {
            result.push_back(entry.name);
        }
        return result;
    }
    
    std::vector<FormDataValue> values() const {
        std::vector<FormDataValue> result;
        for (const auto& entry : entries_) {
            result.push_back(entry.value);
        }
        return result;
    }
    
    // Serialize to multipart/form-data
    std::string toMultipartBody(std::string& boundary) const {
        boundary = "----ZepraFormBoundary" + std::to_string(rand());
        
        std::string body;
        for (const auto& entry : entries_) {
            body += "--" + boundary + "\r\n";
            
            if (std::holds_alternative<std::string>(entry.value)) {
                body += "Content-Disposition: form-data; name=\"" + entry.name + "\"\r\n";
                body += "\r\n";
                body += std::get<std::string>(entry.value) + "\r\n";
            } else {
                const File& file = std::get<File>(entry.value);
                body += "Content-Disposition: form-data; name=\"" + entry.name + "\"; ";
                body += "filename=\"" + file.name() + "\"\r\n";
                body += "Content-Type: " + (file.type().empty() ? "application/octet-stream" : file.type()) + "\r\n";
                body += "\r\n";
                body += file.text() + "\r\n";
            }
        }
        
        body += "--" + boundary + "--\r\n";
        return body;
    }
    
    // Serialize to application/x-www-form-urlencoded (strings only)
    std::string toURLEncoded() const {
        std::string result;
        for (size_t i = 0; i < entries_.size(); i++) {
            if (i > 0) result += "&";
            
            const auto& entry = entries_[i];
            result += encode(entry.name) + "=";
            
            if (std::holds_alternative<std::string>(entry.value)) {
                result += encode(std::get<std::string>(entry.value));
            }
        }
        return result;
    }
    
private:
    static std::string encode(const std::string& str) {
        std::string result;
        for (char c : str) {
            if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
                result += c;
            } else if (c == ' ') {
                result += '+';
            } else {
                char buf[4];
                snprintf(buf, sizeof(buf), "%%%02X", static_cast<unsigned char>(c));
                result += buf;
            }
        }
        return result;
    }
    
    std::vector<FormDataEntry> entries_;
};

} // namespace Zepra::API
