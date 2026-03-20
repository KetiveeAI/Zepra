// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace zepra {

class WebKernel {
public:
    WebKernel();
    ~WebKernel();

    // Handle request/response
    nlohmann::json handleRequest(const nlohmann::json& request);
    void sendResponse(const nlohmann::json& response);
    // Translate between backend/server and UI
    nlohmann::json translateToUI(const nlohmann::json& backendData);
    nlohmann::json translateToBackend(const nlohmann::json& uiData);
};

} // namespace zepra 