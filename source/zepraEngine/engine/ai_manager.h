// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <functional>

namespace zepra {

class AIManager {
public:
    AIManager();
    ~AIManager();

    // AI-powered recommendations
    std::vector<std::string> getRecommendations(const std::string& context) const;
    // AI-powered autofill
    nlohmann::json getAutoFillSuggestions(const nlohmann::json& formData) const;
    // Integrate with backend AI
    void setBackendCallback(std::function<void(const nlohmann::json&)> cb);
};

} // namespace zepra 