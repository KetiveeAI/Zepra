/**
 * @file navigation_controller.cpp
 * @brief Navigation implementation
 */

#include "browser/navigation_controller.h"
#include <iostream>
#include <chrono>

namespace ZepraBrowser {

struct NavigationController::Impl {
    int tabId;
    std::vector<NavigationEntry> history;
    int currentIndex = -1;
    bool loading = false;
    
    NavigationCallback onStart;
    NavigationCallback onComplete;
};

NavigationController::NavigationController(int tabId) 
    : impl_(std::make_unique<Impl>()) {
    impl_->tabId = tabId;
    std::cout << "[NavigationController] Created for tab " << tabId << std::endl;
}

NavigationController::~NavigationController() = default;

void NavigationController::navigateTo(const std::string& url) {
    std::cout << "[NavigationController] Navigating to: " << url << std::endl;
    
    impl_->loading = true;
    
    if (impl_->onStart) {
        impl_->onStart(url);
    }
    
    // Add to history
    NavigationEntry entry;
    entry.url = url;
    entry.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    
    // Clear forward history when navigating
    if (impl_->currentIndex < (int)impl_->history.size() - 1) {
        impl_->history.erase(impl_->history.begin() + impl_->currentIndex + 1, 
                            impl_->history.end());
    }
    
    impl_->history.push_back(entry);
    impl_->currentIndex = impl_->history.size() - 1;
    
    impl_->loading = false;
    
    if (impl_->onComplete) {
        impl_->onComplete(url);
    }
}

bool NavigationController::canGoBack() const {
    return impl_->currentIndex > 0;
}

bool NavigationController::canGoForward() const {
    return impl_->currentIndex < (int)impl_->history.size() - 1;
}

void NavigationController::goBack() {
    if (!canGoBack()) {
        return;
    }
    
    impl_->currentIndex--;
    const std::string& url = impl_->history[impl_->currentIndex].url;
    
    std::cout << "[NavigationController] Going back to: " << url << std::endl;
    
    if (impl_->onStart) {
        impl_->onStart(url);
    }
    if (impl_->onComplete) {
        impl_->onComplete(url);
    }
}

void NavigationController::goForward() {
    if (!canGoForward()) {
        return;
    }
    
    impl_->currentIndex++;
    const std::string& url = impl_->history[impl_->currentIndex].url;
    
    std::cout << "[NavigationController] Going forward to: " << url << std::endl;
    
    if (impl_->onStart) {
        impl_->onStart(url);
    }
    if (impl_->onComplete) {
        impl_->onComplete(url);
    }
}

void NavigationController::reload() {
    if (impl_->currentIndex < 0 || impl_->currentIndex >= impl_->history.size()) {
        return;
    }
    
    const std::string& url = impl_->history[impl_->currentIndex].url;
    
    std::cout << "[NavigationController] Reloading: " << url << std::endl;
    
    if (impl_->onStart) {
        impl_->onStart(url);
    }
    if (impl_->onComplete) {
        impl_->onComplete(url);
    }
}

void NavigationController::stop() {
    impl_->loading = false;
    std::cout << "[NavigationController] Stopped loading" << std::endl;
}

std::string NavigationController::getCurrentUrl() const {
    if (impl_->currentIndex >= 0 && impl_->currentIndex < impl_->history.size()) {
        return impl_->history[impl_->currentIndex].url;
    }
    return "";
}

std::string NavigationController::getCurrentTitle() const {
    if (impl_->currentIndex >= 0 && impl_->currentIndex < impl_->history.size()) {
        return impl_->history[impl_->currentIndex].title;
    }
    return "";
}

bool NavigationController::isLoading() const {
    return impl_->loading;
}

std::vector<NavigationEntry> NavigationController::getHistory() const {
    return impl_->history;
}

void NavigationController::clearHistory() {
    impl_->history.clear();
    impl_->currentIndex = -1;
    std::cout << "[NavigationController] Cleared history" << std::endl;
}

void NavigationController::onNavigationStart(NavigationCallback callback) {
    impl_->onStart = callback;
}

void NavigationController::onNavigationComplete(NavigationCallback callback) {
    impl_->onComplete = callback;
}

} // namespace ZepraBrowser
