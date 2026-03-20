// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
#ifndef ZEPRA_AUTH_UI_H
#define ZEPRA_AUTH_UI_H

#include "auth/zepra_auth.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace ZepraUI {

// UI element types
enum class UIElementType {
    BUTTON,
    TEXT_INPUT,
    LABEL,
    CHECKBOX,
    DROPDOWN,
    MODAL
};

// UI element base class
class UIElement {
public:
    UIElement(int x, int y, int width, int height);
    virtual ~UIElement() = default;
    
    virtual void render(SDL_Renderer* renderer) = 0;
    virtual bool handleEvent(const SDL_Event& event) = 0;
    virtual void update() = 0;
    
    void setPosition(int x, int y);
    void setSize(int width, int height);
    void setVisible(bool visible);
    void setEnabled(bool enabled);
    
    int getX() const { return m_x; }
    int getY() const { return m_y; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    bool isVisible() const { return m_visible; }
    bool isEnabled() const { return m_enabled; }

protected:
    int m_x, m_y, m_width, m_height;
    bool m_visible;
    bool m_enabled;
    bool m_hovered;
    bool m_focused;
};

// Button class
class Button : public UIElement {
public:
    Button(int x, int y, int width, int height, const std::string& text);
    ~Button() override;
    
    void render(SDL_Renderer* renderer) override;
    bool handleEvent(const SDL_Event& event) override;
    void update() override;
    
    void setText(const std::string& text);
    void setOnClick(std::function<void()> callback);
    void setColors(SDL_Color normal, SDL_Color hover, SDL_Color pressed);
    
    const std::string& getText() const { return m_text; }

private:
    std::string m_text;
    std::function<void()> m_onClick;
    SDL_Color m_normalColor;
    SDL_Color m_hoverColor;
    SDL_Color m_pressedColor;
    bool m_pressed;
};

// Text input class
class TextInput : public UIElement {
public:
    TextInput(int x, int y, int width, int height, const std::string& placeholder = "");
    ~TextInput() override;
    
    void render(SDL_Renderer* renderer) override;
    bool handleEvent(const SDL_Event& event) override;
    void update() override;
    
    void setText(const std::string& text);
    void setPlaceholder(const std::string& placeholder);
    void setPasswordMode(bool password);
    void setOnTextChange(std::function<void(const std::string&)> callback);
    void setOnEnter(std::function<void()> callback);
    
    const std::string& getText() const { return m_text; }
    bool isPasswordMode() const { return m_passwordMode; }

private:
    std::string m_text;
    std::string m_placeholder;
    bool m_passwordMode;
    std::function<void(const std::string&)> m_onTextChange;
    std::function<void()> m_onEnter;
    int m_cursorPos;
    bool m_showCursor;
    Uint32 m_cursorBlinkTime;
};

// Label class
class Label : public UIElement {
public:
    Label(int x, int y, const std::string& text);
    ~Label() override;
    
    void render(SDL_Renderer* renderer) override;
    bool handleEvent(const SDL_Event& event) override;
    void update() override;
    
    void setText(const std::string& text);
    void setColor(SDL_Color color);
    void setFontSize(int size);
    
    const std::string& getText() const { return m_text; }

private:
    std::string m_text;
    SDL_Color m_color;
    int m_fontSize;
};

// Modal dialog base class
class ModalDialog {
public:
    ModalDialog(int x, int y, int width, int height, const std::string& title);
    virtual ~ModalDialog() = default;
    
    virtual void render(SDL_Renderer* renderer);
    virtual bool handleEvent(const SDL_Event& event);
    virtual void update();
    
    void setVisible(bool visible);
    bool isVisible() const { return m_visible; }
    
    void setOnClose(std::function<void()> callback);

protected:
    int m_x, m_y, m_width, m_height;
    std::string m_title;
    bool m_visible;
    std::function<void()> m_onClose;
    std::vector<std::unique_ptr<UIElement>> m_elements;
    
    void addElement(std::unique_ptr<UIElement> element);
    void clearElements();
};

// Login dialog
class LoginDialog : public ModalDialog {
public:
    LoginDialog();
    ~LoginDialog() override;
    
    void render(SDL_Renderer* renderer) override;
    bool handleEvent(const SDL_Event& event) override;
    void update() override;
    
    void setOnLogin(std::function<void(const std::string&, const std::string&)> callback);
    void setOnCancel(std::function<void()> callback);
    void setError(const std::string& error);
    void clearError();
    
    void setEmail(const std::string& email);
    void setPassword(const std::string& password);

private:
    std::function<void(const std::string&, const std::string&)> m_onLogin;
    std::function<void()> m_onCancel;
    std::string m_errorMessage;
    
    TextInput* m_emailInput;
    TextInput* m_passwordInput;
    Button* m_loginButton;
    Button* m_cancelButton;
    Label* m_errorLabel;
    Label* m_titleLabel;
    Label* m_emailLabel;
    Label* m_passwordLabel;
};

// 2FA dialog
class TwoFactorDialog : public ModalDialog {
public:
    TwoFactorDialog();
    ~TwoFactorDialog() override;
    
    void render(SDL_Renderer* renderer) override;
    bool handleEvent(const SDL_Event& event) override;
    void update() override;
    
    void setOnVerify(std::function<void(const std::string&)> callback);
    void setOnCancel(std::function<void()> callback);
    void setError(const std::string& error);
    void clearError();
    
    void setTempToken(const std::string& token);

private:
    std::function<void(const std::string&)> m_onVerify;
    std::function<void()> m_onCancel;
    std::string m_errorMessage;
    std::string m_tempToken;
    
    TextInput* m_codeInput;
    Button* m_verifyButton;
    Button* m_cancelButton;
    Label* m_errorLabel;
    Label* m_titleLabel;
    Label* m_instructionLabel;
};

// Password prompt dialog
class PasswordPromptDialog : public ModalDialog {
public:
    PasswordPromptDialog(const std::string& websiteUrl, const std::string& domain);
    ~PasswordPromptDialog() override;
    
    void render(SDL_Renderer* renderer) override;
    bool handleEvent(const SDL_Event& event) override;
    void update() override;
    
    void setOnSubmit(std::function<void(const std::string&, const std::string&)> callback);
    void setOnCancel(std::function<void()> callback);
    void setError(const std::string& error);
    void clearError();

private:
    std::string m_websiteUrl;
    std::string m_domain;
    std::function<void(const std::string&, const std::string&)> m_onSubmit;
    std::function<void()> m_onCancel;
    std::string m_errorMessage;
    
    TextInput* m_usernameInput;
    TextInput* m_passwordInput;
    Button* m_submitButton;
    Button* m_cancelButton;
    Label* m_errorLabel;
    Label* m_titleLabel;
    Label* m_websiteLabel;
    Label* m_usernameLabel;
    Label* m_passwordLabel;
};

// Authentication UI manager
class AuthUIManager {
public:
    static AuthUIManager& getInstance();
    
    bool initialize(SDL_Renderer* renderer, TTF_Font* font);
    void shutdown();
    
    void render();
    bool handleEvent(const SDL_Event& event);
    void update();
    
    // Dialog management
    void showLoginDialog();
    void hideLoginDialog();
    void showTwoFactorDialog(const std::string& tempToken);
    void hideTwoFactorDialog();
    void showPasswordPromptDialog(const std::string& websiteUrl, const std::string& domain);
    void hidePasswordPromptDialog();
    
    // Callbacks
    void setOnLogin(std::function<void(const std::string&, const std::string&)> callback);
    void setOnTwoFactor(std::function<void(const std::string&)> callback);
    void setOnPasswordPrompt(std::function<void(const std::string&, const std::string&)> callback);
    
    // Error handling
    void setLoginError(const std::string& error);
    void setTwoFactorError(const std::string& error);
    void setPasswordPromptError(const std::string& error);

private:
    AuthUIManager();
    ~AuthUIManager();
    
    SDL_Renderer* m_renderer;
    TTF_Font* m_font;
    
    std::unique_ptr<LoginDialog> m_loginDialog;
    std::unique_ptr<TwoFactorDialog> m_twoFactorDialog;
    std::unique_ptr<PasswordPromptDialog> m_passwordPromptDialog;
    
    // Callbacks
    std::function<void(const std::string&, const std::string&)> m_onLogin;
    std::function<void(const std::string&)> m_onTwoFactor;
    std::function<void(const std::string&, const std::string&)> m_onPasswordPrompt;
    
    // Disable copy constructor and assignment
    AuthUIManager(const AuthUIManager&) = delete;
    AuthUIManager& operator=(const AuthUIManager&) = delete;
};

// Utility functions
namespace AuthUIUtils {
    SDL_Color createColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255);
    void renderText(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, 
                   int x, int y, SDL_Color color);
    void renderRect(SDL_Renderer* renderer, int x, int y, int width, int height, 
                   SDL_Color color, bool filled = true);
    void renderBorder(SDL_Renderer* renderer, int x, int y, int width, int height, 
                     SDL_Color color, int thickness = 1);
    bool isPointInRect(int x, int y, int rectX, int rectY, int rectWidth, int rectHeight);
    std::string maskPassword(const std::string& password);
}

// Colors
namespace Colors {
    const SDL_Color WHITE = {255, 255, 255, 255};
    const SDL_Color BLACK = {0, 0, 0, 255};
    const SDL_Color GRAY = {128, 128, 128, 255};
    const SDL_Color LIGHT_GRAY = {192, 192, 192, 255};
    const SDL_Color DARK_GRAY = {64, 64, 64, 255};
    const SDL_Color BLUE = {0, 122, 255, 255};
    const SDL_Color GREEN = {0, 255, 0, 255};
    const SDL_Color RED = {255, 0, 0, 255};
    const SDL_Color YELLOW = {255, 255, 0, 255};
    const SDL_Color TRANSPARENT = {0, 0, 0, 0};
}

} // namespace ZepraUI

#endif // ZEPRA_AUTH_UI_H 