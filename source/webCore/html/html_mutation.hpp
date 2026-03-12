/**
 * @file html_mutation.hpp
 * @brief Mutation Observer API
 */

#pragma once

#include "html/html_element.hpp"
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace Zepra::WebCore {

/**
 * @brief Mutation record type
 */
enum class MutationRecordType {
    Attributes,
    CharacterData,
    ChildList
};

/**
 * @brief Mutation record
 */
struct MutationRecord {
    MutationRecordType type;
    HTMLElement* target = nullptr;
    std::vector<HTMLElement*> addedNodes;
    std::vector<HTMLElement*> removedNodes;
    HTMLElement* previousSibling = nullptr;
    HTMLElement* nextSibling = nullptr;
    std::string attributeName;
    std::string attributeNamespace;
    std::string oldValue;
};

/**
 * @brief Mutation observer options
 */
struct MutationObserverInit {
    bool childList = false;
    bool attributes = false;
    bool characterData = false;
    bool subtree = false;
    bool attributeOldValue = false;
    bool characterDataOldValue = false;
    std::vector<std::string> attributeFilter;
};

/**
 * @brief Mutation observer
 */
class MutationObserver {
public:
    using Callback = std::function<void(const std::vector<MutationRecord>&, MutationObserver*)>;
    
    explicit MutationObserver(Callback callback);
    ~MutationObserver();
    
    void observe(HTMLElement* target, const MutationObserverInit& options);
    void disconnect();
    std::vector<MutationRecord> takeRecords();
    
    // Internal use
    void enqueueRecord(const MutationRecord& record);
    void deliver();
    
private:
    Callback callback_;
    std::vector<std::pair<HTMLElement*, MutationObserverInit>> targets_;
    std::vector<MutationRecord> recordQueue_;
};

/**
 * @brief Resize observer entry
 */
struct ResizeObserverEntry {
    HTMLElement* target;
    float contentWidth;
    float contentHeight;
    float borderBoxWidth;
    float borderBoxHeight;
};

/**
 * @brief Resize observer
 */
class ResizeObserver {
public:
    using Callback = std::function<void(const std::vector<ResizeObserverEntry>&, ResizeObserver*)>;
    
    explicit ResizeObserver(Callback callback);
    ~ResizeObserver();
    
    void observe(HTMLElement* target);
    void unobserve(HTMLElement* target);
    void disconnect();
    
private:
    Callback callback_;
    std::vector<HTMLElement*> targets_;
};

/**
 * @brief Intersection observer options
 */
struct IntersectionObserverInit {
    HTMLElement* root = nullptr;
    std::string rootMargin = "0px";
    std::vector<double> threshold = {0};
};

/**
 * @brief Intersection observer entry
 */
struct IntersectionObserverEntry {
    double time;
    HTMLElement* target;
    float boundingClientX;
    float boundingClientY;
    float boundingClientWidth;
    float boundingClientHeight;
    float intersectionX;
    float intersectionY;
    float intersectionWidth;
    float intersectionHeight;
    float rootX;
    float rootY;
    float rootWidth;
    float rootHeight;
    double intersectionRatio;
    bool isIntersecting;
};

/**
 * @brief Intersection observer
 */
class IntersectionObserver {
public:
    using Callback = std::function<void(const std::vector<IntersectionObserverEntry>&, IntersectionObserver*)>;
    
    IntersectionObserver(Callback callback, const IntersectionObserverInit& options = {});
    ~IntersectionObserver();
    
    HTMLElement* root() const { return options_.root; }
    std::string rootMargin() const { return options_.rootMargin; }
    const std::vector<double>& thresholds() const { return options_.threshold; }
    
    void observe(HTMLElement* target);
    void unobserve(HTMLElement* target);
    void disconnect();
    std::vector<IntersectionObserverEntry> takeRecords();
    
private:
    Callback callback_;
    IntersectionObserverInit options_;
    std::vector<HTMLElement*> targets_;
    std::vector<IntersectionObserverEntry> recordQueue_;
};

} // namespace Zepra::WebCore
