/**
 * @file html_drag_drop.hpp
 * @brief Drag and Drop API
 */

#pragma once

#include "html/html_element.hpp"
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

namespace Zepra::WebCore {

/**
 * @brief Drop effect
 */
enum class DropEffect {
    None,
    Copy,
    Link,
    Move
};

/**
 * @brief Effective allowed operations
 */
enum class EffectAllowed {
    None,
    Copy,
    CopyLink,
    CopyMove,
    Link,
    LinkMove,
    Move,
    All,
    Uninitialized
};

/**
 * @brief Data transfer item kind
 */
enum class DataTransferItemKind {
    String,
    File
};

/**
 * @brief Data transfer item
 */
class DataTransferItem {
public:
    DataTransferItemKind kind() const { return kind_; }
    std::string type() const { return type_; }
    
    void getAsString(std::function<void(const std::string&)> callback) const;
    // File* getAsFile() const; // For file items
    
private:
    DataTransferItemKind kind_;
    std::string type_;
    std::string data_;
    
    friend class DataTransfer;
};

/**
 * @brief Data transfer item list
 */
class DataTransferItemList {
public:
    size_t length() const { return items_.size(); }
    DataTransferItem* operator[](size_t index) { return &items_[index]; }
    
    DataTransferItem* add(const std::string& data, const std::string& type);
    void remove(size_t index);
    void clear();
    
private:
    std::vector<DataTransferItem> items_;
    
    friend class DataTransfer;
};

/**
 * @brief File list for drag/drop
 */
class FileList {
public:
    size_t length() const { return files_.size(); }
    
    struct FileInfo {
        std::string name;
        std::string type;
        size_t size;
        std::string path;
    };
    
    const FileInfo* item(size_t index) const {
        return index < files_.size() ? &files_[index] : nullptr;
    }
    
    void add(const FileInfo& file) { files_.push_back(file); }
    
private:
    std::vector<FileInfo> files_;
};

/**
 * @brief Data transfer (clipboard/drag)
 */
class DataTransfer {
public:
    DataTransfer();
    ~DataTransfer();
    
    // Properties
    DropEffect dropEffect() const { return dropEffect_; }
    void setDropEffect(DropEffect e) { dropEffect_ = e; }
    
    EffectAllowed effectAllowed() const { return effectAllowed_; }
    void setEffectAllowed(EffectAllowed e) { effectAllowed_ = e; }
    
    DataTransferItemList& items() { return items_; }
    std::vector<std::string> types() const;
    FileList& files() { return files_; }
    
    // Methods
    void setData(const std::string& format, const std::string& data);
    std::string getData(const std::string& format) const;
    void clearData(const std::string& format = "");
    
    void setDragImage(HTMLElement* image, int x, int y);
    
private:
    DropEffect dropEffect_ = DropEffect::None;
    EffectAllowed effectAllowed_ = EffectAllowed::Uninitialized;
    DataTransferItemList items_;
    FileList files_;
    std::unordered_map<std::string, std::string> data_;
    
    HTMLElement* dragImage_ = nullptr;
    int dragImageX_ = 0;
    int dragImageY_ = 0;
};

/**
 * @brief Drag event
 */
struct DragEvent {
    HTMLElement* target = nullptr;
    HTMLElement* relatedTarget = nullptr;
    DataTransfer* dataTransfer = nullptr;
    int clientX = 0;
    int clientY = 0;
    int screenX = 0;
    int screenY = 0;
    bool ctrlKey = false;
    bool shiftKey = false;
    bool altKey = false;
    bool metaKey = false;
};

/**
 * @brief Draggable mixin for elements
 */
class Draggable {
public:
    virtual ~Draggable() = default;
    
    bool draggable() const { return draggable_; }
    void setDraggable(bool d) { draggable_ = d; }
    
    // Event handlers
    std::function<void(DragEvent&)> onDragStart;
    std::function<void(DragEvent&)> onDrag;
    std::function<void(DragEvent&)> onDragEnd;
    std::function<void(DragEvent&)> onDragEnter;
    std::function<void(DragEvent&)> onDragOver;
    std::function<void(DragEvent&)> onDragLeave;
    std::function<void(DragEvent&)> onDrop;
    
private:
    bool draggable_ = false;
};

} // namespace Zepra::WebCore
