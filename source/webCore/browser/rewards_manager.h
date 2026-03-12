// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file rewards_manager.h
 * @brief Ketivee rewards and points system
 */

#pragma once

#include <string>
#include <cstdint>
#include <chrono>
#include <vector>
#include <functional>
#include <memory>

namespace ZepraBrowser {

enum class RewardType {
    DailyLogin,        // Daily login bonus
    Browse,            // Points per page viewed
    Search,            // Points per search
    Referral,          // Referred new user
    Achievement,       // Special achievements
    Bonus              // Special promotions
};

struct RewardEvent {
    RewardType type;
    int32_t points;
    std::string description;
    std::chrono::system_clock::time_point timestamp;
};

struct UserPoints {
    int64_t totalPoints = 0;
    int64_t lifetimePoints = 0;
    int32_t level = 1;
    std::chrono::system_clock::time_point lastDailyReward;
    int32_t dailyStreak = 0;
};

/**
 * RewardsManager - Ketivee points and rewards system
 * 
 * Features:
 * - Daily login rewards (streak bonuses)
 * - Browse & search points
 * - Referral system
 * - Point redemption
 * - Achievements
 */
class RewardsManager {
public:
    static RewardsManager& instance();
    
    // User account
    void setUserId(const std::string& userId);
    std::string getUserId() const;
    bool isLoggedIn() const;
    
    // Points
    UserPoints getPoints() const;
    void addPoints(RewardType type, int32_t amount, const std::string& desc = "");
    bool redeemPoints(int32_t amount, const std::string& item);
    
    // Daily rewards
    bool canClaimDailyReward() const;
    int32_t claimDailyReward();  // Returns points earned
   
    
    // Activity tracking
    void onPageView(const std::string& url);
    void onSearch(const std::string& query);
    void onReferral(const std::string& referredUserId);
    
    // History
    std::vector<RewardEvent> getRecentRewards(int count = 10) const;
    
    // Sync with server
    void syncWithServer();
    bool needsSync() const;
    
    // Callbacks
    using PointsCallback = std::function<void(int32_t newTotal)>;
    void onPointsChanged(PointsCallback callback);
    
private:
    RewardsManager();
    ~RewardsManager();
    RewardsManager(const RewardsManager&) = delete;
    RewardsManager& operator=(const RewardsManager&) = delete;
    
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace ZepraBrowser
