// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
#include "../../source/zepraEngine/include/engine/json_bridge.h"
#include <iostream>

namespace zepra {

JsonBridge::JsonBridge() {}
JsonBridge::~JsonBridge() {}

nlohmann::json JsonBridge::parse(const std::string& jsonStr) const {
    try {
        return nlohmann::json::parse(jsonStr);
    } catch (const std::exception& e) {
        std::cerr << "JSON parse error: " << e.what() << std::endl;
        return nlohmann::json();
    }
}

std::string JsonBridge::serialize(const nlohmann::json& j) const {
    try {
        return j.dump();
    } catch (const std::exception& e) {
        std::cerr << "JSON serialize error: " << e.what() << std::endl;
        return "";
    }
}

void JsonBridge::sendToUI(const nlohmann::json& j) const {
    // TODO: Implement UI communication
    std::cout << "[JsonBridge] Send to UI: " << j.dump() << std::endl;
}

void JsonBridge::sendToBackend(const nlohmann::json& j) const {
    // TODO: Implement backend communication
    std::cout << "[JsonBridge] Send to Backend: " << j.dump() << std::endl;
}

nlohmann::json JsonBridge::receiveFromUI() const {
    // TODO: Implement UI receive
    return nlohmann::json();
}

nlohmann::json JsonBridge::receiveFromBackend() const {
    // TODO: Implement backend receive
    return nlohmann::json();
}

} // namespace zepra 