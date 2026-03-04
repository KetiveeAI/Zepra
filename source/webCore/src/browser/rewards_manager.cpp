/**
 * @file rewards_manager.cpp
 * @brief Rewards system implementation
 */

#include "browser/rewards_manager.h"
#include <iostream>
#include <fstream>
#include <map>
#include <algorithm>

namespace ZepraBrowser {

// Points configuration
constexpr int32_t DAILY_LOGIN_BASE = 10;
constexpr int32_t DAILY_STREAK_BONUS = 5;  // +5 per day streak
constexpr int32_t PAGE_VIEW_POINTS = 1;
constexpr int32_t SEARCH_POINTS = 2;
constexpr int32_t REFERRAL_POINTS = 100;

struct RewardsManager::Impl {
    std::string userId;
    UserPoints points;
    std::vector<RewardEvent> recentEvents;
    PointsCallback pointsCallback;
    bool needsSync = false;
    
    // Rate limiting
    std::chrono::system_clock::time_point lastPageView;
    std::chrono::system_clock::time_point lastSearch;
};

RewardsManager& RewardsManager::instance() {
    static RewardsManager instance;
    return instance;
}

RewardsManager::RewardsManager() : impl_(std::make_unique<Impl>()) {
    std::cout << "[RewardsManager] Initialized" << std::endl;
}

RewardsManager::~RewardsManager() = default;

void RewardsManager::setUserId(const std::string& userId) {
    impl_->userId = userId;
    impl_->needsSync = true;
    std::cout << "[RewardsManager] User ID set: " << userId << std::endl;
}

std::string RewardsManager::getUserId() const {
    return impl_->userId;
}

bool RewardsManager::isLoggedIn() const {
    return !impl_->userId.empty();
}

UserPoints RewardsManager::getPoints() const {
    return impl_->points;
}

void RewardsManager::addPoints(RewardType type, int32_t amount, const std::string& desc) {
    impl_->points.totalPoints += amount;
    impl_->points.lifetimePoints += amount;
    
    // Level up every 1000 points
    impl_->points.level = 1 + (impl_->points.lifetimePoints / 1000);
    
    // Record event
    RewardEvent event;
    event.type = type;
    event.points = amount;
    event.description = desc;
    event.timestamp = std::chrono::system_clock::now();
    
    impl_->recentEvents.insert(impl_->recentEvents.begin(), event);
    if (impl_->recentEvents.size() > 100) {
        impl_->recentEvents.resize(100);
    }
    
    impl_->needsSync = true;
    
    std::cout << "[RewardsManager] +" << amount << " points: " << desc 
              << " (Total: " << impl_->points.totalPoints << ")" << std::endl;
    
    if (impl_->pointsCallback) {
        impl_->pointsCallback(impl_->points.totalPoints);
    }
}

bool RewardsManager::redeemPoints(int32_t amount, const std::string& item) {
    if (impl_->points.totalPoints < amount) {
        std::cout << "[RewardsManager] Insufficient points for: " << item << std::endl;
        return false;
    }
    
    impl_->points.totalPoints -= amount;
    impl_->needsSync = true;
    
    std::cout << "[RewardsManager] Redeemed " << amount << " points for: " << item << std::endl;
    
    if (impl_->pointsCallback) {
        impl_->pointsCallback(impl_->points.totalPoints);
    }
    
    return true;
}

bool RewardsManager::canClaimDailyReward() const {
    auto now = std::chrono::system_clock::now();
    auto diff = now - impl_->points.lastDailyReward;
    auto hours = std::chrono::duration_cast<std::chrono::hours>(diff).count();
    
    return hours >= 24;
}

int32_t RewardsManager::claimDailyReward() {
    if (!canClaimDailyReward()) {
        std::cout << "[RewardsManager] Daily reward already claimed" << std::endl;
        return 0;
    }
    
    auto now = std::chrono::system_clock::now();
    auto diff = now - impl_->points.lastDailyReward;
    auto hours = std::chrono::duration_cast<std::chrono::hours>(diff).count();
    
    // Check if streak continues (claimed within 48 hours)
    if (hours < 48) {
        impl_->points.dailyStreak++;
    } else {
        impl_->points.dailyStreak = 1;  // Reset streak
    }
    
    impl_->points.lastDailyReward = now;
    
    // Calculate reward
    int32_t basePoints = DAILY_LOGIN_BASE;
    int32_t streakBonus = (impl_->points.dailyStreak - 1) * DAILY_STREAK_BONUS;
    int32_t total = basePoints + streakBonus;
    
    std::string desc = "Daily login (Day " + std::to_string(impl_->points.dailyStreak) + ")";
    addPoints(RewardType::DailyLogin, total, desc);
    
    return total;
}

void RewardsManager::onPageView(const std::string& url) {
    if (!isLoggedIn()) return;
    
    auto now = std::chrono::system_clock::now();
    auto diff = now - impl_->lastPageView;
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(diff).count();
    
    // Rate limit: 1 point per 10 seconds
    if (seconds >= 10) {
        addPoints(RewardType::Browse, PAGE_VIEW_POINTS, "Page view");
        impl_->lastPageView = now;
    }
}

void RewardsManager::onSearch(const std::string& query) {
    if (!isLoggedIn()) return;
    
    auto now = std::chrono::system_clock::now();
    auto diff = now - impl_->lastSearch;
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(diff).count();
    
    // Rate limit: 2 points per 30 seconds
    if (seconds >= 30) {
        addPoints(RewardType::Search, SEARCH_POINTS, "Search: " + query.substr(0, 20));
        impl_->lastSearch = now;
    }
}

void RewardsManager::onReferral(const std::string& referredUserId) {
    if (!isLoggedIn()) return;
    
    addPoints(RewardType::Referral, REFERRAL_POINTS, "Referral: " + referredUserId);
}

std::vector<RewardEvent> RewardsManager::getRecentRewards(int count) const {
    std::vector<RewardEvent> result;
    int n = std::min(count, (int)impl_->recentEvents.size());
    
    for (int i = 0; i < n; i++) {
        result.push_back(impl_->recentEvents[i]);
    }
    
    return result;
}

void RewardsManager::syncWithServer() {
    if (!isLoggedIn() || !impl_->needsSync) {
        return;
    }
    
    // TODO: HTTP POST to Ketivee API
    std::cout << "[RewardsManager] Syncing points with server..." << std::endl;
    
    impl_->needsSync = false;
}

bool RewardsManager::needsSync() const {
    return impl_->needsSync;
}

void RewardsManager::onPointsChanged(PointsCallback callback) {
    impl_->pointsCallback = callback;
}

} // namespace ZepraBrowser
