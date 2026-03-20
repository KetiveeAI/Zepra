// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file auth_ui.cpp
 * @brief Authentication UI stub implementations
 */

#include "ui/auth_ui.h"

namespace ZepraUI {

// AuthUIManager singleton implementation
AuthUIManager& AuthUIManager::getInstance() {
    static AuthUIManager instance;
    return instance;
}

// Constructor and destructor
AuthUIManager::AuthUIManager() 
    : m_renderer(nullptr), m_font(nullptr) {
}

AuthUIManager::~AuthUIManager() {
    shutdown();
}

// Core lifecycle
bool AuthUIManager::initialize(SDL_Renderer* renderer, TTF_Font* font) {
    m_renderer = renderer;
    m_font = font;
    return true;
}

void AuthUIManager::shutdown() {
    // Cleanup stubs
}

void AuthUIManager::update() {
    // Update stubs
}

void AuthUIManager::render() {
    // Render stubs
}

bool AuthUIManager::handleEvent(const SDL_Event& event) {
    return false;
}

// Dialog control stubs
void AuthUIManager::showLoginDialog() {
    // Stub - would show login dialog UI
}

void AuthUIManager::showTwoFactorDialog(const std::string& tempToken) {
    // Stub
}

void AuthUIManager::showPasswordPromptDialog(const std::string& url, const std::string& domain) {
    // Stub
}

void AuthUIManager::hideLoginDialog() {
    // Stub
}

void AuthUIManager::hideTwoFactorDialog() {
    // Stub
}

void AuthUIManager::hidePasswordPromptDialog() {
    // Stub
}

void AuthUIManager::setLoginError(const std::string& error) {
    // Stub - would set error on login dialog
}

void AuthUIManager::setTwoFactorError(const std::string& error) {
    // Stub - would set error on 2FA dialog
}

void AuthUIManager::setPasswordPromptError(const std::string& error) {
    // Stub - would set error on password prompt dialog
}

// Callback setters
void AuthUIManager::setOnLogin(std::function<void(const std::string&, const std::string&)> callback) {
    m_onLogin = callback;
}

void AuthUIManager::setOnTwoFactor(std::function<void(const std::string&)> callback) {
    m_onTwoFactor = callback;
}

void AuthUIManager::setOnPasswordPrompt(std::function<void(const std::string&, const std::string&)> callback) {
    m_onPasswordPrompt = callback;
}

} // namespace ZepraUI
