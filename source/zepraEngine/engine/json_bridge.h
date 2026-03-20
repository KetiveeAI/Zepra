// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace zepra {

class JsonBridge {
public:
    JsonBridge();
    ~JsonBridge();

    // Parse JSON string to nlohmann::json
    nlohmann::json parse(const std::string& jsonStr) const;
    // Serialize nlohmann::json to string
    std::string serialize(const nlohmann::json& j) const;
    // Send JSON message to UI/backend
    void sendToUI(const nlohmann::json& j) const;
    void sendToBackend(const nlohmann::json& j) const;
    // Receive JSON message from UI/backend
    nlohmann::json receiveFromUI() const;
    nlohmann::json receiveFromBackend() const;
};

} // namespace zepra 