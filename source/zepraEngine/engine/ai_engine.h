// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
#pragma once

#include "../common/types.h"
#include "chat_html_parser.h"
#include <nlohmann/json.hpp>
#include <chrono>
#include <optional>
#include <regex>
#include <unordered_map>

namespace zepra {

struct AISummarizeResult {
    String summary;
    std::vector<String> keywords;
};

struct AIRecommendation {
    String title;
    String url;
    String description;
    float confidence;
};

struct AIAutofillPrediction {
    String suggestion;
    float confidence;
};

class AIEngine {
public:
    AIEngine();

    AISummarizeResult summarizeTab(const String& htmlContent);
    std::vector<AIRecommendation> generateRecommendations(const String& input);
    std::vector<AIAutofillPrediction> predictAutofill(const String& input);

    nlohmann::json processRoute(const String& route, const nlohmann::json& payload);

private:
    bool isLikelyUrl(const String& text) const;
    String extractDomain(const String& text) const;
    String sanitizeInput(const String& text) const;

    AISummarizeResult summarizeFromText(const String& text);
    std::vector<AIRecommendation> recommendFromUrl(const String& url);
    std::vector<AIRecommendation> recommendFromText(const String& text);

    std::vector<String> tokenize(const String& text) const;

    std::chrono::steady_clock::time_point lastSummaryTime_;
    std::unordered_map<String, std::chrono::steady_clock::time_point> summaryThrottle_;
};

} // namespace zepra
