/**
 * @file html_clipboard.hpp
 * @brief Clipboard API
 */

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <cstdint>
#include <memory>

namespace Zepra::WebCore {

/**
 * @brief Clipboard item
 */
class ClipboardItem {
public:
    ClipboardItem(const std::string& type, const std::vector<uint8_t>& data);
    ~ClipboardItem() = default;
    
    std::vector<std::string> types() const { return types_; }
    
    void getType(const std::string& type, 
                 std::function<void(const std::vector<uint8_t>&)> callback) const;
    
private:
    std::vector<std::string> types_;
    std::vector<std::vector<uint8_t>> data_;
};

/**
 * @brief Clipboard API
 */
class Clipboard {
public:
    Clipboard() = default;
    ~Clipboard() = default;
    
    // Async Clipboard API
    void read(std::function<void(const std::vector<ClipboardItem>&)> callback);
    void readText(std::function<void(const std::string&)> callback);
    
    void write(const std::vector<ClipboardItem>& items,
               std::function<void(bool)> callback);
    void writeText(const std::string& text,
                   std::function<void(bool)> callback);
    
    // Events
    std::function<void()> onClipboardChange;
    
private:
    std::vector<ClipboardItem> items_;
};

/**
 * @brief Clipboard event
 */
struct ClipboardEvent {
    std::string type;  // "copy", "cut", "paste"
    class DataTransfer* clipboardData = nullptr;
};

} // namespace Zepra::WebCore
