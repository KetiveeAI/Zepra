/**
 * @file browser_context.cpp
 * @brief Browser context implementation
 */

#include "integration/browser_context.hpp"
#include "zeprascript/script_engine.hpp"
#include <algorithm>

namespace Zepra::Integration {

BrowserContext::BrowserContext() = default;
BrowserContext::~BrowserContext() = default;

void BrowserContext::initialize(WebCore::DOMDocument* doc, WebCore::PageRenderer* renderer) {
    document_ = doc;
    renderer_ = renderer;
    
    // Create DOM bridge (engine created separately)
    bridge_ = std::make_unique<DOMBridge>(engine_.get(), renderer);
    bridge_->setDocument(doc);
    bridge_->initialize();
    
    // Setup browser globals
    setupGlobals();
}

void BrowserContext::setupGlobals() {
    // Global setup would happen here
}

std::vector<ScriptInfo> BrowserContext::collectScripts() {
    std::vector<ScriptInfo> scripts;
    // Would traverse document looking for <script> tags
    return scripts;
}

void BrowserContext::executeScripts() {
    auto scripts = collectScripts();
    
    // Sort: sync first, then defer
    std::stable_sort(scripts.begin(), scripts.end(), [](const auto& a, const auto& b) {
        if (a.defer != b.defer) return !a.defer;
        if (a.async != b.async) return !a.async;
        return a.order < b.order;
    });
    
    for (const auto& script : scripts) {
        if (script.async) continue;
        executeScript(script.source, script.src.empty() ? "<inline>" : script.src);
    }
}

void BrowserContext::executeScript(const std::string& source, const std::string& filename) {
    if (source.empty()) return;
    
    // Would execute through engine
    // engine_->execute(source, filename);
    
    // Request DOM update
    bridge_->requestUpdate();
}

void BrowserContext::fireDOMContentLoaded() {
    executeScript("if (window.onDOMContentLoaded) window.onDOMContentLoaded();", "<event>");
    bridge_->requestUpdate();
}

void BrowserContext::fireLoad() {
    executeScript("if (window.onload) window.onload();", "<event>");
    bridge_->requestUpdate();
}

void BrowserContext::tick(double timestamp) {
    uint64_t now = static_cast<uint64_t>(timestamp);
    
    // Process timers
    std::vector<TimerInfo> triggeredTimers;
    
    for (auto it = timers_.begin(); it != timers_.end();) {
        if (now >= it->triggerTime) {
            triggeredTimers.push_back(*it);
            if (it->repeat) {
                it->triggerTime = now + it->interval;
                ++it;
            } else {
                it = timers_.erase(it);
            }
        } else {
            ++it;
        }
    }
    
    for (auto& timer : triggeredTimers) {
        timer.callback();
    }
    
    // Process animation frames
    auto frames = std::move(animFrames_);
    animFrames_.clear();
    
    for (auto& [id, callback] : frames) {
        callback(timestamp);
    }
    
    // Update rendering if needed
    bridge_->requestUpdate();
}

bool BrowserContext::hasPendingWork() const {
    return !timers_.empty() || !animFrames_.empty();
}

uint32_t BrowserContext::setTimeout(std::function<void()> callback, uint32_t delay) {
    uint32_t id = nextTimerId_++;
    auto now = std::chrono::steady_clock::now();
    uint64_t nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    
    timers_.push_back({id, std::move(callback), nowMs + delay, delay, false});
    return id;
}

uint32_t BrowserContext::setInterval(std::function<void()> callback, uint32_t delay) {
    uint32_t id = nextTimerId_++;
    auto now = std::chrono::steady_clock::now();
    uint64_t nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    
    timers_.push_back({id, std::move(callback), nowMs + delay, delay, true});
    return id;
}

void BrowserContext::clearTimeout(uint32_t id) {
    timers_.erase(
        std::remove_if(timers_.begin(), timers_.end(),
            [id](const TimerInfo& t) { return t.id == id; }),
        timers_.end());
}

uint32_t BrowserContext::requestAnimationFrame(std::function<void(double)> callback) {
    uint32_t id = nextAnimFrameId_++;
    animFrames_.emplace_back(id, std::move(callback));
    return id;
}

void BrowserContext::cancelAnimationFrame(uint32_t id) {
    animFrames_.erase(
        std::remove_if(animFrames_.begin(), animFrames_.end(),
            [id](const auto& p) { return p.first == id; }),
        animFrames_.end());
}

// Factory
std::unique_ptr<BrowserContext> BrowserContextFactory::create() {
    return std::make_unique<BrowserContext>();
}

std::unique_ptr<BrowserContext> BrowserContextFactory::createForPage(
    WebCore::DOMDocument* doc, 
    WebCore::PageRenderer* renderer) {
    auto ctx = std::make_unique<BrowserContext>();
    ctx->initialize(doc, renderer);
    return ctx;
}

} // namespace Zepra::Integration
