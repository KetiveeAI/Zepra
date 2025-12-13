/**
 * @file window.cpp
 * @brief Window, Console, Screen, Storage, Performance implementation
 */

#include "webcore/browser/window.hpp"
#include "webcore/dom.hpp"
#include "webcore/html/html_element.hpp"
#include <chrono>
#include <queue>
#include <iostream>
#include <cmath>
#include <unordered_map>

namespace Zepra::WebCore {

// =============================================================================
// Screen
// =============================================================================

class Screen::Impl {
public:
    unsigned int width = 1920;
    unsigned int height = 1080;
    unsigned int availWidth = 1920;
    unsigned int availHeight = 1040;
    unsigned int colorDepth = 24;
    std::string orientation = "landscape-primary";
};

Screen::Screen() : impl_(std::make_unique<Impl>()) {}
Screen::~Screen() = default;

unsigned int Screen::width() const { return impl_->width; }
unsigned int Screen::height() const { return impl_->height; }
unsigned int Screen::availWidth() const { return impl_->availWidth; }
unsigned int Screen::availHeight() const { return impl_->availHeight; }
unsigned int Screen::colorDepth() const { return impl_->colorDepth; }
unsigned int Screen::pixelDepth() const { return impl_->colorDepth; }
std::string Screen::orientation() const { return impl_->orientation; }

// =============================================================================
// Console
// =============================================================================

class Console::Impl {
public:
    int groupLevel = 0;
    std::unordered_map<std::string, int> counters;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> timers;
    
    void output(const std::string& level, const std::string& msg) {
        std::string indent(groupLevel * 2, ' ');
        std::cerr << "[" << level << "] " << indent << msg << std::endl;
    }
};

Console::Console() : impl_(std::make_unique<Impl>()) {}
Console::~Console() = default;

void Console::log(const std::string& message) { impl_->output("LOG", message); }
void Console::info(const std::string& message) { impl_->output("INFO", message); }
void Console::warn(const std::string& message) { impl_->output("WARN", message); }
void Console::error(const std::string& message) { impl_->output("ERROR", message); }
void Console::debug(const std::string& message) { impl_->output("DEBUG", message); }
void Console::trace(const std::string& message) { impl_->output("TRACE", message); }

void Console::group(const std::string& label) {
    if (!label.empty()) impl_->output("GROUP", label);
    impl_->groupLevel++;
}

void Console::groupCollapsed(const std::string& label) {
    group(label);
}

void Console::groupEnd() {
    if (impl_->groupLevel > 0) impl_->groupLevel--;
}

void Console::time(const std::string& label) {
    impl_->timers[label] = std::chrono::steady_clock::now();
}

void Console::timeLog(const std::string& label) {
    auto it = impl_->timers.find(label);
    if (it != impl_->timers.end()) {
        auto elapsed = std::chrono::steady_clock::now() - it->second;
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
        impl_->output("TIMER", label + ": " + std::to_string(ms) + "ms");
    }
}

void Console::timeEnd(const std::string& label) {
    timeLog(label);
    impl_->timers.erase(label);
}

void Console::count(const std::string& label) {
    int count = ++impl_->counters[label];
    impl_->output("COUNT", label + ": " + std::to_string(count));
}

void Console::countReset(const std::string& label) {
    impl_->counters[label] = 0;
}

void Console::clear() {
    std::cerr << "\033[2J\033[H";  // ANSI clear screen
}

void Console::table(const std::string& data) {
    impl_->output("TABLE", data);
}

void Console::assert_(bool condition, const std::string& message) {
    if (!condition) {
        impl_->output("ASSERT", message.empty() ? "Assertion failed" : message);
    }
}

// =============================================================================
// Performance
// =============================================================================

class Performance::Impl {
public:
    std::chrono::steady_clock::time_point timeOrigin;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> marks;
    
    Impl() : timeOrigin(std::chrono::steady_clock::now()) {}
};

Performance::Performance() : impl_(std::make_unique<Impl>()) {}
Performance::~Performance() = default;

double Performance::now() const {
    auto elapsed = std::chrono::steady_clock::now() - impl_->timeOrigin;
    return std::chrono::duration<double, std::milli>(elapsed).count();
}

double Performance::timeOrigin() const {
    auto epoch = impl_->timeOrigin.time_since_epoch();
    return std::chrono::duration<double, std::milli>(epoch).count();
}

void Performance::mark(const std::string& name) {
    impl_->marks[name] = std::chrono::steady_clock::now();
}

void Performance::measure(const std::string& /*name*/, 
                          const std::string& /*startMark*/,
                          const std::string& /*endMark*/) {
    // Would create performance measure entry
}

void Performance::clearMarks(const std::string& name) {
    if (name.empty()) {
        impl_->marks.clear();
    } else {
        impl_->marks.erase(name);
    }
}

void Performance::clearMeasures(const std::string& /*name*/) {
    // Would clear measure entries
}

// =============================================================================
// Storage
// =============================================================================

class Storage::Impl {
public:
    std::vector<std::pair<std::string, std::string>> items;
    
    int findIndex(const std::string& key) {
        for (size_t i = 0; i < items.size(); ++i) {
            if (items[i].first == key) return static_cast<int>(i);
        }
        return -1;
    }
};

Storage::Storage() : impl_(std::make_unique<Impl>()) {}
Storage::~Storage() = default;

size_t Storage::length() const {
    return impl_->items.size();
}

std::string Storage::key(size_t index) const {
    if (index < impl_->items.size()) {
        return impl_->items[index].first;
    }
    return "";
}

std::string Storage::getItem(const std::string& key) const {
    int idx = const_cast<Storage::Impl*>(impl_.get())->findIndex(key);
    if (idx >= 0) {
        return impl_->items[idx].second;
    }
    return "";
}

void Storage::setItem(const std::string& key, const std::string& value) {
    int idx = impl_->findIndex(key);
    if (idx >= 0) {
        impl_->items[idx].second = value;
    } else {
        impl_->items.push_back({key, value});
    }
}

void Storage::removeItem(const std::string& key) {
    int idx = impl_->findIndex(key);
    if (idx >= 0) {
        impl_->items.erase(impl_->items.begin() + idx);
    }
}

void Storage::clear() {
    impl_->items.clear();
}

// =============================================================================
// Window::Impl
// =============================================================================

struct TimerEntry {
    int id;
    TimerCallback callback;
    std::chrono::steady_clock::time_point fireTime;
    int interval;  // 0 for timeout, >0 for interval
    bool canceled = false;
};

class Window::Impl {
public:
    DOMDocument* document = nullptr;
    std::unique_ptr<Location> location;
    std::unique_ptr<History> history;
    std::unique_ptr<Navigator> navigator;
    std::unique_ptr<Screen> screen;
    std::unique_ptr<Console> console;
    std::unique_ptr<Performance> performance;
    std::unique_ptr<Storage> localStorage;
    std::unique_ptr<Storage> sessionStorage;
    
    std::string name;
    std::string status;
    bool closed = false;
    double devicePixelRatio = 1.0;
    int innerWidth = 1920;
    int innerHeight = 1080;
    int outerWidth = 1920;
    int outerHeight = 1080;
    double scrollX = 0;
    double scrollY = 0;
    int screenX = 0;
    int screenY = 0;
    
    Window* parent = nullptr;
    Window* opener = nullptr;
    std::vector<Window*> frames;
    
    // Timers
    int nextTimerId = 1;
    std::vector<TimerEntry> timers;
    
    // Animation frames
    int nextAnimFrameId = 1;
    std::vector<std::pair<int, AnimationFrameCallback>> animFrames;
    
    // Event handlers
    std::unordered_map<std::string, EventListener> handlers;
};

// =============================================================================
// Window
// =============================================================================

Window::Window() : impl_(std::make_unique<Impl>()) {
    impl_->location = std::make_unique<Location>(this);
    impl_->history = std::make_unique<History>(this);
    impl_->navigator = std::make_unique<Navigator>(this);
    impl_->screen = std::make_unique<Screen>();
    impl_->console = std::make_unique<Console>();
    impl_->performance = std::make_unique<Performance>();
    impl_->localStorage = std::make_unique<Storage>();
    impl_->sessionStorage = std::make_unique<Storage>();
}

Window::~Window() = default;

// Core properties
DOMDocument* Window::document() { return impl_->document; }
const DOMDocument* Window::document() const { return impl_->document; }
void Window::setDocument(DOMDocument* doc) { impl_->document = doc; }

Location* Window::location() { return impl_->location.get(); }
const Location* Window::location() const { return impl_->location.get(); }

History* Window::history() { return impl_->history.get(); }
const History* Window::history() const { return impl_->history.get(); }

Navigator* Window::navigator() { return impl_->navigator.get(); }
const Navigator* Window::navigator() const { return impl_->navigator.get(); }

Screen* Window::screen() { return impl_->screen.get(); }
Console* Window::console() { return impl_->console.get(); }
Performance* Window::performance() { return impl_->performance.get(); }
Storage* Window::localStorage() { return impl_->localStorage.get(); }
Storage* Window::sessionStorage() { return impl_->sessionStorage.get(); }

// Window properties
std::string Window::name() const { return impl_->name; }
void Window::setName(const std::string& name) { impl_->name = name; }

std::string Window::status() const { return impl_->status; }
void Window::setStatus(const std::string& status) { impl_->status = status; }

bool Window::closed() const { return impl_->closed; }
double Window::devicePixelRatio() const { return impl_->devicePixelRatio; }
int Window::innerWidth() const { return impl_->innerWidth; }
int Window::innerHeight() const { return impl_->innerHeight; }
int Window::outerWidth() const { return impl_->outerWidth; }
int Window::outerHeight() const { return impl_->outerHeight; }
double Window::pageXOffset() const { return impl_->scrollX; }
double Window::pageYOffset() const { return impl_->scrollY; }
double Window::scrollX() const { return impl_->scrollX; }
double Window::scrollY() const { return impl_->scrollY; }
int Window::screenX() const { return impl_->screenX; }
int Window::screenY() const { return impl_->screenY; }
bool Window::isSecureContext() const { return impl_->location->protocol() == "https:"; }
std::string Window::origin() const { return impl_->location->origin(); }

// Frame properties
Window* Window::self() { return this; }
Window* Window::window() { return this; }
Window* Window::top() { 
    Window* w = this;
    while (w->impl_->parent) w = w->impl_->parent;
    return w;
}
Window* Window::parent() { return impl_->parent ? impl_->parent : this; }
Window* Window::opener() { return impl_->opener; }
HTMLElement* Window::frameElement() { return nullptr; }
size_t Window::length() const { return impl_->frames.size(); }
Window* Window::frame(size_t index) { 
    return index < impl_->frames.size() ? impl_->frames[index] : nullptr;
}

// Timers
int Window::setTimeout(TimerCallback callback, int delay) {
    int id = impl_->nextTimerId++;
    auto fireTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(delay);
    impl_->timers.push_back({id, std::move(callback), fireTime, 0, false});
    return id;
}

void Window::clearTimeout(int id) {
    for (auto& t : impl_->timers) {
        if (t.id == id) t.canceled = true;
    }
}

int Window::setInterval(TimerCallback callback, int delay) {
    int id = impl_->nextTimerId++;
    auto fireTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(delay);
    impl_->timers.push_back({id, std::move(callback), fireTime, delay, false});
    return id;
}

void Window::clearInterval(int id) {
    clearTimeout(id);
}

int Window::requestAnimationFrame(AnimationFrameCallback callback) {
    int id = impl_->nextAnimFrameId++;
    impl_->animFrames.push_back({id, std::move(callback)});
    return id;
}

void Window::cancelAnimationFrame(int id) {
    impl_->animFrames.erase(
        std::remove_if(impl_->animFrames.begin(), impl_->animFrames.end(),
                       [id](const auto& p) { return p.first == id; }),
        impl_->animFrames.end());
}

int Window::requestIdleCallback(TimerCallback callback, int timeout) {
    // Simplified: just use setTimeout
    return setTimeout(std::move(callback), timeout > 0 ? timeout : 1);
}

void Window::cancelIdleCallback(int id) {
    clearTimeout(id);
}

void Window::processTimers() {
    auto now = std::chrono::steady_clock::now();
    
    // Process timers
    for (size_t i = 0; i < impl_->timers.size(); ) {
        auto& t = impl_->timers[i];
        if (t.canceled) {
            impl_->timers.erase(impl_->timers.begin() + i);
            continue;
        }
        
        if (t.fireTime <= now) {
            if (t.callback) t.callback();
            
            if (t.interval > 0) {
                // Reschedule interval
                t.fireTime = now + std::chrono::milliseconds(t.interval);
                ++i;
            } else {
                // Remove one-shot timeout
                impl_->timers.erase(impl_->timers.begin() + i);
            }
        } else {
            ++i;
        }
    }
    
    // Process animation frames
    if (!impl_->animFrames.empty()) {
        auto frames = std::move(impl_->animFrames);
        impl_->animFrames.clear();
        
        double timestamp = impl_->performance->now();
        for (auto& [id, cb] : frames) {
            if (cb) cb(timestamp);
        }
    }
}

// Dialogs
void Window::alert(const std::string& message) {
    impl_->console->log("ALERT: " + message);
}

bool Window::confirm(const std::string& message) {
    impl_->console->log("CONFIRM: " + message);
    return true;  // Would show actual dialog
}

std::string Window::prompt(const std::string& message, const std::string& defaultValue) {
    impl_->console->log("PROMPT: " + message);
    return defaultValue;  // Would show actual dialog
}

void Window::print() {
    // Would trigger print dialog
}

// Window control
void Window::close() {
    impl_->closed = true;
}

void Window::stop() {
    // Would stop page loading
}

void Window::focus() {
    Event event("focus", true, false);
    dispatchEvent(event);
}

void Window::blur() {
    Event event("blur", true, false);
    dispatchEvent(event);
}

Window* Window::open(const std::string& /*url*/, 
                     const std::string& /*target*/,
                     const std::string& /*features*/) {
    // Would open new window
    return nullptr;
}

void Window::moveTo(int x, int y) {
    impl_->screenX = x;
    impl_->screenY = y;
}

void Window::moveBy(int dx, int dy) {
    impl_->screenX += dx;
    impl_->screenY += dy;
}

void Window::resizeTo(int width, int height) {
    impl_->outerWidth = width;
    impl_->outerHeight = height;
}

void Window::resizeBy(int dw, int dh) {
    impl_->outerWidth += dw;
    impl_->outerHeight += dh;
}

void Window::scroll(double x, double y) {
    impl_->scrollX = x;
    impl_->scrollY = y;
    
    Event event("scroll", true, false);
    dispatchEvent(event);
}

void Window::scrollTo(double x, double y) {
    scroll(x, y);
}

void Window::scrollBy(double dx, double dy) {
    scroll(impl_->scrollX + dx, impl_->scrollY + dy);
}

CSSStyleDeclaration* Window::getComputedStyle(HTMLElement* element, const std::string& /*pseudoElt*/) {
    if (element) {
        return element->style();
    }
    return nullptr;
}

bool Window::matchMedia(const std::string& query) {
    // Simplified media query matching
    if (query.find("prefers-color-scheme: dark") != std::string::npos) {
        return true;  // Assume dark mode
    }
    return false;
}

// Encoding
std::string Window::atob(const std::string& encoded) {
    // Base64 decode
    static const std::string b64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    
    std::vector<int> T(256, -1);
    for (size_t i = 0; i < 64; ++i) T[static_cast<unsigned char>(b64[i])] = static_cast<int>(i);
    
    int val = 0, bits = -8;
    for (char c : encoded) {
        if (T[static_cast<unsigned char>(c)] == -1) break;
        val = (val << 6) + T[static_cast<unsigned char>(c)];
        bits += 6;
        if (bits >= 0) {
            result += static_cast<char>((val >> bits) & 0xFF);
            bits -= 8;
        }
    }
    return result;
}

std::string Window::btoa(const std::string& data) {
    // Base64 encode
    static const char* b64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    
    for (size_t i = 0; i < data.size(); i += 3) {
        int n = (static_cast<unsigned char>(data[i]) << 16);
        if (i + 1 < data.size()) n |= (static_cast<unsigned char>(data[i+1]) << 8);
        if (i + 2 < data.size()) n |= static_cast<unsigned char>(data[i+2]);
        
        result += b64[(n >> 18) & 63];
        result += b64[(n >> 12) & 63];
        result += (i + 1 < data.size()) ? b64[(n >> 6) & 63] : '=';
        result += (i + 2 < data.size()) ? b64[n & 63] : '=';
    }
    return result;
}

// Event handlers
#define DEFINE_WINDOW_EVENT(name, eventName) \
    void Window::setOn##name(EventListener callback) { \
        impl_->handlers[eventName] = std::move(callback); \
        addEventListener(eventName, impl_->handlers[eventName]); \
    }

DEFINE_WINDOW_EVENT(Load, "load")
DEFINE_WINDOW_EVENT(Unload, "unload")
DEFINE_WINDOW_EVENT(BeforeUnload, "beforeunload")
DEFINE_WINDOW_EVENT(Error, "error")
DEFINE_WINDOW_EVENT(Resize, "resize")
DEFINE_WINDOW_EVENT(Scroll, "scroll")
DEFINE_WINDOW_EVENT(Focus, "focus")
DEFINE_WINDOW_EVENT(Blur, "blur")
DEFINE_WINDOW_EVENT(Online, "online")
DEFINE_WINDOW_EVENT(Offline, "offline")
DEFINE_WINDOW_EVENT(HashChange, "hashchange")
DEFINE_WINDOW_EVENT(PopState, "popstate")
DEFINE_WINDOW_EVENT(Storage, "storage")
DEFINE_WINDOW_EVENT(Message, "message")

#undef DEFINE_WINDOW_EVENT

} // namespace Zepra::WebCore
