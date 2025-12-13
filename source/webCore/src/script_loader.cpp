/**
 * @file script_loader.cpp
 * @brief Script loading with async/defer support per HTML5 spec
 */

#include "webcore/script_loader.hpp"
#include "webcore/script_context.hpp"
#include <algorithm>
#include <iostream>

namespace Zepra::WebCore {

// =============================================================================
// ScriptLoader Implementation
// =============================================================================

ScriptLoader::ScriptLoader(ScriptContext* context)
    : context_(context) {}

ScriptLoader::~ScriptLoader() = default;

void ScriptLoader::setResourceLoader(ResourceLoader* loader) {
    resourceLoader_ = loader;
}

ScriptLoadMode ScriptLoader::determineLoadMode(DOMElement* element) const {
    if (!element) return ScriptLoadMode::Blocking;
    
    // Check type="module" first
    std::string type = element->getAttribute("type");
    if (type == "module") {
        return ScriptLoadMode::Module;  // Modules are always deferred
    }
    
    // Check for async attribute
    if (element->hasAttribute("async")) {
        return ScriptLoadMode::Async;
    }
    
    // Check for defer attribute
    if (element->hasAttribute("defer")) {
        return ScriptLoadMode::Defer;
    }
    
    // Default: blocking
    return ScriptLoadMode::Blocking;
}

std::string ScriptLoader::getInlineContent(DOMElement* element) const {
    if (!element) return "";
    
    std::string content;
    for (auto& child : element->childNodes()) {
        if (child->nodeType() == NodeType::Text) {
            content += static_cast<DOMText*>(child.get())->data();
        }
    }
    return content;
}

bool ScriptLoader::processScript(DOMElement* element, bool& parserBlocked) {
    parserBlocked = false;
    if (!element || element->tagName() != "script") return false;
    
    PendingScript script;
    script.element = element;
    script.documentOrder = documentOrder_++;
    script.mode = determineLoadMode(element);
    script.isModule = (element->getAttribute("type") == "module");
    
    // Check for external src
    std::string src = element->getAttribute("src");
    if (!src.empty()) {
        script.url = src;
        script.isLoaded = false;
    } else {
        // Inline script
        script.inlineContent = getInlineContent(element);
        script.isLoaded = true;  // Inline is always "loaded"
    }
    
    // Ignore empty scripts
    if (script.url.empty() && script.inlineContent.empty()) {
        return true;  // Nothing to execute
    }
    
    switch (script.mode) {
        case ScriptLoadMode::Blocking:
            // Parser-blocking: execute immediately or wait for load
            if (script.isLoaded) {
                // Inline blocking script - execute now
                executeScript(script);
            } else {
                // External blocking script - load then execute
                blockingScripts_.push_back(std::move(script));
                currentBlockingScript_ = &blockingScripts_.back();
                loadExternalScript(blockingScripts_.back());
                parserBlocked = true;
            }
            break;
            
        case ScriptLoadMode::Defer:
        case ScriptLoadMode::Module:
            // Deferred: queue for later, don't block
            deferredScripts_.push_back(std::move(script));
            if (!deferredScripts_.back().isLoaded) {
                loadExternalScript(deferredScripts_.back());
            }
            break;
            
        case ScriptLoadMode::Async:
            // Async: load and execute whenever ready
            asyncScripts_.push_back(std::move(script));
            if (!asyncScripts_.back().isLoaded) {
                loadExternalScript(asyncScripts_.back());
            }
            break;
    }
    
    return true;
}

void ScriptLoader::loadExternalScript(PendingScript& script) {
    if (!resourceLoader_ || script.url.empty()) return;
    
    // Start async load
    std::thread([this, &script]() {
        Response response = resourceLoader_->loadURL(script.url);
        
        if (response.ok()) {
            script.inlineContent = response.text();
            script.isLoaded = true;
            
            // If async, mark ready
            if (script.mode == ScriptLoadMode::Async) {
                std::lock_guard<std::mutex> lock(asyncMutex_);
                readyAsyncScripts_.push_back(&script);
            }
        } else {
            std::cerr << "Failed to load script: " << script.url << std::endl;
            script.isLoaded = true;  // Mark loaded so we don't block forever
        }
    }).detach();
}

bool ScriptLoader::executeScript(PendingScript& script) {
    if (!context_ || script.isExecuted) return false;
    
    // Mark as executed before running (prevent re-entry)
    script.isExecuted = true;
    
    if (!script.inlineContent.empty()) {
        context_->evaluate(script.inlineContent);
        return true;
    }
    
    return false;
}

void ScriptLoader::executeBlockingScripts() {
    // Process any ready blocking scripts
    for (auto& script : blockingScripts_) {
        if (script.isLoaded && !script.isExecuted) {
            executeScript(script);
        }
    }
    
    // Clear executed scripts
    blockingScripts_.erase(
        std::remove_if(blockingScripts_.begin(), blockingScripts_.end(),
            [](const PendingScript& s) { return s.isExecuted; }),
        blockingScripts_.end()
    );
    
    // Update current blocking script
    if (blockingScripts_.empty()) {
        currentBlockingScript_ = nullptr;
    } else {
        currentBlockingScript_ = &blockingScripts_.front();
    }
}

void ScriptLoader::onDOMContentLoaded() {
    domReady_ = true;
    
    // Sort deferred scripts by document order
    std::sort(deferredScripts_.begin(), deferredScripts_.end(),
        [](const PendingScript& a, const PendingScript& b) {
            return a.documentOrder < b.documentOrder;
        });
    
    // Execute all deferred scripts in order
    // Wait for each to be loaded before executing
    for (auto& script : deferredScripts_) {
        // Spin-wait for load (simplified - real impl should use events)
        int attempts = 0;
        while (!script.isLoaded && attempts < 1000) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            attempts++;
        }
        
        if (script.isLoaded && !script.isExecuted) {
            executeScript(script);
        }
    }
    
    deferredScripts_.clear();
}

void ScriptLoader::onLoad() {
    loadComplete_ = true;
}

void ScriptLoader::processAsyncQueue() {
    std::vector<PendingScript*> ready;
    
    {
        std::lock_guard<std::mutex> lock(asyncMutex_);
        ready.swap(readyAsyncScripts_);
    }
    
    // Execute ready async scripts
    for (auto* script : ready) {
        if (script && !script->isExecuted) {
            executeScript(*script);
        }
    }
    
    // Clean up executed async scripts
    asyncScripts_.erase(
        std::remove_if(asyncScripts_.begin(), asyncScripts_.end(),
            [](const PendingScript& s) { return s.isExecuted; }),
        asyncScripts_.end()
    );
}

bool ScriptLoader::isBlocked() const {
    // Parser is blocked if there's an unexecuted blocking script
    for (const auto& script : blockingScripts_) {
        if (!script.isExecuted) {
            if (!script.isLoaded) return true;  // Waiting for load
        }
    }
    return false;
}

size_t ScriptLoader::pendingCount() const {
    return blockingScripts_.size() + deferredScripts_.size() + asyncScripts_.size();
}

// =============================================================================
// ScriptInsertionPoint Implementation
// =============================================================================

void ScriptInsertionPoint::push(DOMElement* insertionPoint) {
    stack_.push_back(insertionPoint);
}

void ScriptInsertionPoint::pop() {
    if (!stack_.empty()) {
        stack_.pop_back();
    }
}

DOMElement* ScriptInsertionPoint::current() const {
    return stack_.empty() ? nullptr : stack_.back();
}

bool ScriptInsertionPoint::isActive() const {
    return !stack_.empty();
}

} // namespace Zepra::WebCore
