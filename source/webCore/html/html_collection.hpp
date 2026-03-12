/**
 * @file html_collection.hpp
 * @brief HTML Collections - NodeList, HTMLCollection, etc.
 */

#pragma once

#include "html/html_element.hpp"
#include <vector>
#include <string>
#include <functional>

namespace Zepra::WebCore {

/**
 * @brief Base collection interface
 */
class HTMLCollection {
public:
    virtual ~HTMLCollection() = default;
    
    virtual size_t length() const = 0;
    virtual HTMLElement* item(size_t index) const = 0;
    virtual HTMLElement* namedItem(const std::string& name) const = 0;
    
    HTMLElement* operator[](size_t index) const { return item(index); }
    
    // Iterator support
    class Iterator {
    public:
        Iterator(const HTMLCollection* col, size_t idx) : col_(col), idx_(idx) {}
        HTMLElement* operator*() const { return col_->item(idx_); }
        Iterator& operator++() { ++idx_; return *this; }
        bool operator!=(const Iterator& o) const { return idx_ != o.idx_; }
    private:
        const HTMLCollection* col_;
        size_t idx_;
    };
    
    Iterator begin() const { return Iterator(this, 0); }
    Iterator end() const { return Iterator(this, length()); }
};

/**
 * @brief Live HTML Collection
 */
class LiveHTMLCollection : public HTMLCollection {
public:
    using Matcher = std::function<bool(HTMLElement*)>;
    
    LiveHTMLCollection(HTMLElement* root, Matcher matcher);
    ~LiveHTMLCollection() override = default;
    
    size_t length() const override;
    HTMLElement* item(size_t index) const override;
    HTMLElement* namedItem(const std::string& name) const override;
    
    void invalidate() { cached_ = false; }
    
private:
    HTMLElement* root_;
    Matcher matcher_;
    mutable std::vector<HTMLElement*> cache_;
    mutable bool cached_ = false;
    
    void updateCache() const;
};

/**
* @brief Static Node List
 */
class StaticNodeList : public HTMLCollection {
public:
    StaticNodeList() = default;
    explicit StaticNodeList(std::vector<HTMLElement*> nodes);
    ~StaticNodeList() override = default;
    
    size_t length() const override { return nodes_.size(); }
    HTMLElement* item(size_t index) const override;
    HTMLElement* namedItem(const std::string& name) const override;
    
    void add(HTMLElement* node) { nodes_.push_back(node); }
    void clear() { nodes_.clear(); }
    
private:
    std::vector<HTMLElement*> nodes_;
};

/**
 * @brief Form Controls Collection
 */
class HTMLFormControlsCollection : public HTMLCollection {
public:
    explicit HTMLFormControlsCollection(HTMLElement* form);
    ~HTMLFormControlsCollection() override = default;
    
    size_t length() const override;
    HTMLElement* item(size_t index) const override;
    HTMLElement* namedItem(const std::string& name) const override;
    
private:
    HTMLElement* form_;
    mutable std::vector<HTMLElement*> cache_;
    mutable bool cached_ = false;
};

/**
 * @brief Options Collection (for select elements)
 */
class HTMLOptionsCollection : public HTMLCollection {
public:
    explicit HTMLOptionsCollection(HTMLElement* select);
    ~HTMLOptionsCollection() override = default;
    
    size_t length() const override;
    HTMLElement* item(size_t index) const override;
    HTMLElement* namedItem(const std::string& name) const override;
    
    void add(HTMLElement* option, int before = -1);
    void remove(int index);
    
    int selectedIndex() const;
    void setSelectedIndex(int index);
    
private:
    HTMLElement* select_;
};

/**
 * @brief Radios Collection (for radio button groups)
 */
class RadioNodeList : public HTMLCollection {
public:
    RadioNodeList(HTMLElement* form, const std::string& name);
    ~RadioNodeList() override = default;
    
    size_t length() const override;
    HTMLElement* item(size_t index) const override;
    HTMLElement* namedItem(const std::string& name) const override;
    
    std::string value() const;
    void setValue(const std::string& v);
    
private:
    HTMLElement* form_;
    std::string name_;
    mutable std::vector<HTMLElement*> cache_;
};

} // namespace Zepra::WebCore
