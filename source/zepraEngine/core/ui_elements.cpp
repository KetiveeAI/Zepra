// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
#include "core/zepra_core.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>

namespace ZepraCore {

// UIElement base class implementation
UIElement::UIElement(int x, int y, int width, int height, UIElementType type)
    : m_x(x), m_y(y), m_width(width), m_height(height)
    , m_visible(true), m_enabled(true), m_hovered(false), m_focused(false)
    , m_type(type) {
}

void UIElement::setPosition(int x, int y) {
    m_x = x;
    m_y = y;
}

void UIElement::setSize(int width, int height) {
    m_width = width;
    m_height = height;
}

void UIElement::setVisible(bool visible) {
    m_visible = visible;
}

void UIElement::setEnabled(bool enabled) {
    m_enabled = enabled;
}

// Button implementation
Button::Button(int x, int y, int width, int height, const std::string& text)
    : UIElement(x, y, width, height, UIElementType::BUTTON)
    , m_text(text), m_pressed(false) {
    
    // Set default colors
    m_normalColor = Colors::BLUE;
    m_hoverColor = {0, 100, 200, 255};
    m_pressedColor = {0, 80, 160, 255};
}

Button::~Button() {
}

void Button::render(SDL_Renderer* renderer) {
    if (!m_visible) return;
    
    // Determine color based on state
    SDL_Color color = m_normalColor;
    if (m_pressed) {
        color = m_pressedColor;
    } else if (m_hovered) {
        color = m_hoverColor;
    }
    
    // Render button background
    Utils::renderRect(renderer, m_x, m_y, m_width, m_height, color);
    Utils::renderBorder(renderer, m_x, m_y, m_width, m_height, Colors::DARK_GRAY);
    
    // Render text (centered)
    if (!m_text.empty()) {
        // Calculate text position (centered)
        int textX = m_x + (m_width - m_text.length() * 8) / 2; // Approximate character width
        int textY = m_y + (m_height - 16) / 2; // Approximate character height
        
        Utils::renderText(renderer, nullptr, m_text, textX, textY, Colors::WHITE);
    }
}

bool Button::handleEvent(const SDL_Event& event) {
    if (!m_visible || !m_enabled) return false;
    
    switch (event.type) {
        case SDL_MOUSEMOTION: {
            int mouseX = event.motion.x;
            int mouseY = event.motion.y;
            
            bool wasHovered = m_hovered;
            m_hovered = Utils::isPointInRect(mouseX, mouseY, m_x, m_y, m_width, m_height);
            
            if (wasHovered != m_hovered) {
                // Hover state changed
            }
            break;
        }
        
        case SDL_MOUSEBUTTONDOWN: {
            if (event.button.button == SDL_BUTTON_LEFT) {
                int mouseX = event.button.x;
                int mouseY = event.button.y;
                
                if (Utils::isPointInRect(mouseX, mouseY, m_x, m_y, m_width, m_height)) {
                    m_pressed = true;
                    return true;
                }
            }
            break;
        }
        
        case SDL_MOUSEBUTTONUP: {
            if (event.button.button == SDL_BUTTON_LEFT) {
                int mouseX = event.button.x;
                int mouseY = event.button.y;
                
                if (m_pressed && Utils::isPointInRect(mouseX, mouseY, m_x, m_y, m_width, m_height)) {
                    if (m_onClick) {
                        m_onClick();
                    }
                    m_pressed = false;
                    return true;
                }
                m_pressed = false;
            }
            break;
        }
    }
    
    return false;
}

void Button::update() {
    // Button-specific updates if needed
}

void Button::setText(const std::string& text) {
    m_text = text;
}

void Button::setOnClick(std::function<void()> callback) {
    m_onClick = callback;
}

void Button::setColors(SDL_Color normal, SDL_Color hover, SDL_Color pressed) {
    m_normalColor = normal;
    m_hoverColor = hover;
    m_pressedColor = pressed;
}

// TextInput implementation
TextInput::TextInput(int x, int y, int width, int height, const std::string& placeholder)
    : UIElement(x, y, width, height, UIElementType::TEXT_INPUT)
    , m_text(""), m_placeholder(placeholder), m_passwordMode(false)
    , m_cursorPos(0), m_showCursor(true), m_cursorBlinkTime(SDL_GetTicks()) {
}

TextInput::~TextInput() {
}

void TextInput::render(SDL_Renderer* renderer) {
    if (!m_visible) return;
    
    // Render background
    SDL_Color bgColor = m_focused ? Colors::WHITE : Colors::LIGHT_GRAY;
    Utils::renderRect(renderer, m_x, m_y, m_width, m_height, bgColor);
    Utils::renderBorder(renderer, m_x, m_y, m_width, m_height, Colors::GRAY);
    
    // Render text or placeholder
    std::string displayText = m_text;
    if (displayText.empty() && !m_placeholder.empty()) {
        displayText = m_placeholder;
    }
    
    if (m_passwordMode && !m_text.empty()) {
        displayText = std::string(m_text.length(), '*');
    }
    
    if (!displayText.empty()) {
        Utils::renderText(renderer, nullptr, displayText, m_x + 5, m_y + 5, Colors::BLACK);
    }
    
    // Render cursor if focused
    if (m_focused && m_showCursor) {
        int cursorX = m_x + 5 + (m_cursorPos * 8); // Approximate character width
        Utils::renderRect(renderer, cursorX, m_y + 5, 2, m_height - 10, Colors::BLACK);
    }
}

bool TextInput::handleEvent(const SDL_Event& event) {
    if (!m_visible || !m_enabled) return false;
    
    switch (event.type) {
        case SDL_MOUSEBUTTONDOWN: {
            if (event.button.button == SDL_BUTTON_LEFT) {
                int mouseX = event.button.x;
                int mouseY = event.button.y;
                
                bool wasFocused = m_focused;
                m_focused = Utils::isPointInRect(mouseX, mouseY, m_x, m_y, m_width, m_height);
                
                if (m_focused && !wasFocused) {
                    // Gained focus
                    m_cursorBlinkTime = SDL_GetTicks();
                }
                
                if (m_focused) {
                    return true;
                }
            }
            break;
        }
        
        case SDL_KEYDOWN: {
            if (!m_focused) return false;
            
            switch (event.key.keysym.sym) {
                case SDLK_BACKSPACE: {
                    if (m_cursorPos > 0) {
                        m_text.erase(m_cursorPos - 1, 1);
                        m_cursorPos--;
                        if (m_onTextChange) {
                            m_onTextChange(m_text);
                        }
                    }
                    return true;
                }
                
                case SDLK_DELETE: {
                    if (m_cursorPos < m_text.length()) {
                        m_text.erase(m_cursorPos, 1);
                        if (m_onTextChange) {
                            m_onTextChange(m_text);
                        }
                    }
                    return true;
                }
                
                case SDLK_LEFT: {
                    if (m_cursorPos > 0) {
                        m_cursorPos--;
                    }
                    return true;
                }
                
                case SDLK_RIGHT: {
                    if (m_cursorPos < m_text.length()) {
                        m_cursorPos++;
                    }
                    return true;
                }
                
                case SDLK_HOME: {
                    m_cursorPos = 0;
                    return true;
                }
                
                case SDLK_END: {
                    m_cursorPos = m_text.length();
                    return true;
                }
                
                case SDLK_RETURN:
                case SDLK_KP_ENTER: {
                    if (m_onEnter) {
                        m_onEnter();
                    }
                    return true;
                }
            }
            break;
        }
        
        case SDL_TEXTINPUT: {
            if (!m_focused) return false;
            
            std::string inputText = event.text.text;
            m_text.insert(m_cursorPos, inputText);
            m_cursorPos += inputText.length();
            
            if (m_onTextChange) {
                m_onTextChange(m_text);
            }
            
            return true;
        }
    }
    
    return false;
}

void TextInput::update() {
    // Update cursor blink
    Uint32 currentTime = SDL_GetTicks();
    if (currentTime - m_cursorBlinkTime > 500) { // Blink every 500ms
        m_showCursor = !m_showCursor;
        m_cursorBlinkTime = currentTime;
    }
}

void TextInput::setText(const std::string& text) {
    m_text = text;
    m_cursorPos = m_text.length();
}

void TextInput::setPlaceholder(const std::string& placeholder) {
    m_placeholder = placeholder;
}

void TextInput::setPasswordMode(bool password) {
    m_passwordMode = password;
}

void TextInput::setOnTextChange(std::function<void(const std::string&)> callback) {
    m_onTextChange = callback;
}

void TextInput::setOnEnter(std::function<void()> callback) {
    m_onEnter = callback;
}

// ProgressBar implementation
ProgressBar::ProgressBar(int x, int y, int width, int height)
    : UIElement(x, y, width, height, UIElementType::PROGRESS_BAR)
    , m_progress(0.0), m_text("") {
    
    // Set default colors
    m_backgroundColor = Colors::LIGHT_GRAY;
    m_foregroundColor = Colors::BLUE;
    m_textColor = Colors::BLACK;
}

ProgressBar::~ProgressBar() {
}

void ProgressBar::render(SDL_Renderer* renderer) {
    if (!m_visible) return;
    
    // Render background
    Utils::renderRect(renderer, m_x, m_y, m_width, m_height, m_backgroundColor);
    Utils::renderBorder(renderer, m_x, m_y, m_width, m_height, Colors::GRAY);
    
    // Render progress
    if (m_progress > 0.0) {
        int progressWidth = static_cast<int>(m_width * m_progress);
        Utils::renderRect(renderer, m_x, m_y, progressWidth, m_height, m_foregroundColor);
    }
    
    // Render text
    if (!m_text.empty()) {
        int textX = m_x + (m_width - m_text.length() * 8) / 2;
        int textY = m_y + (m_height - 16) / 2;
        Utils::renderText(renderer, nullptr, m_text, textX, textY, m_textColor);
    }
}

bool ProgressBar::handleEvent(const SDL_Event& event) {
    // Progress bars typically don't handle events
    return false;
}

void ProgressBar::update() {
    // Progress bar updates if needed
}

void ProgressBar::setProgress(double progress) {
    m_progress = std::max(0.0, std::min(1.0, progress));
}

void ProgressBar::setText(const std::string& text) {
    m_text = text;
}

void ProgressBar::setColors(SDL_Color background, SDL_Color foreground, SDL_Color text) {
    m_backgroundColor = background;
    m_foregroundColor = foreground;
    m_textColor = text;
}

// Label implementation
Label::Label(int x, int y, const std::string& text)
    : UIElement(x, y, 0, 0, UIElementType::LABEL)
    , m_text(text), m_color(Colors::BLACK), m_fontSize(16) {
    
    // Calculate size based on text
    m_width = m_text.length() * 8; // Approximate character width
    m_height = m_fontSize;
}

Label::~Label() {
}

void Label::render(SDL_Renderer* renderer) {
    if (!m_visible) return;
    
    Utils::renderText(renderer, nullptr, m_text, m_x, m_y, m_color);
}

bool Label::handleEvent(const SDL_Event& event) {
    // Labels typically don't handle events
    return false;
}

void Label::update() {
    // Label updates if needed
}

void Label::setText(const std::string& text) {
    m_text = text;
    m_width = m_text.length() * 8;
}

void Label::setColor(SDL_Color color) {
    m_color = color;
}

void Label::setFontSize(int size) {
    m_fontSize = size;
    m_height = m_fontSize;
}

// DownloadItem implementation
DownloadItem::DownloadItem(int x, int y, int width, int height, const DownloadInfo& download)
    : UIElement(x, y, width, height, UIElementType::DOWNLOAD_ITEM)
    , m_download(download) {
    
    // Create UI elements for the download item
    int buttonWidth = 60;
    int buttonHeight = 25;
    int spacing = 5;
    
    m_pauseButton = new Button(x + width - buttonWidth * 4 - spacing * 3, y + 5, buttonWidth, buttonHeight, "Pause");
    m_resumeButton = new Button(x + width - buttonWidth * 3 - spacing * 2, y + 5, buttonWidth, buttonHeight, "Resume");
    m_cancelButton = new Button(x + width - buttonWidth * 2 - spacing, y + 5, buttonWidth, buttonHeight, "Cancel");
    m_openButton = new Button(x + width - buttonWidth, y + 5, buttonWidth, buttonHeight, "Open");
    
    m_progressBar = new ProgressBar(x + 10, y + 25, width - 20, 10);
    m_filenameLabel = new Label(x + 10, y + 5, download.filename);
    m_speedLabel = new Label(x + 10, y + 40, "");
    m_statusLabel = new Label(x + 10, y + 55, "");
    
    // Set up button callbacks
    m_pauseButton->setOnClick([this]() {
        if (m_onPause) {
            m_onPause(m_download.id);
        }
    });
    
    m_resumeButton->setOnClick([this]() {
        if (m_onResume) {
            m_onResume(m_download.id);
        }
    });
    
    m_cancelButton->setOnClick([this]() {
        if (m_onCancel) {
            m_onCancel(m_download.id);
        }
    });
    
    m_openButton->setOnClick([this]() {
        if (m_onOpen) {
            m_onOpen(m_download.id);
        }
    });
    
    updateDisplay();
}

DownloadItem::~DownloadItem() {
    delete m_pauseButton;
    delete m_resumeButton;
    delete m_cancelButton;
    delete m_openButton;
    delete m_progressBar;
    delete m_filenameLabel;
    delete m_speedLabel;
    delete m_statusLabel;
}

void DownloadItem::render(SDL_Renderer* renderer) {
    if (!m_visible) return;
    
    // Render background
    Utils::renderRect(renderer, m_x, m_y, m_width, m_height, Colors::LIGHT_GRAY);
    Utils::renderBorder(renderer, m_x, m_y, m_width, m_height, Colors::GRAY);
    
    // Render child elements
    m_filenameLabel->render(renderer);
    m_progressBar->render(renderer);
    m_speedLabel->render(renderer);
    m_statusLabel->render(renderer);
    
    // Render buttons based on download state
    switch (m_download.state) {
        case DownloadState::DOWNLOADING:
            m_pauseButton->render(renderer);
            m_cancelButton->render(renderer);
            break;
            
        case DownloadState::PAUSED:
            m_resumeButton->render(renderer);
            m_cancelButton->render(renderer);
            break;
            
        case DownloadState::COMPLETED:
            m_openButton->render(renderer);
            break;
            
        case DownloadState::FAILED:
        case DownloadState::CANCELLED:
            // No buttons for failed/cancelled downloads
            break;
            
        default:
            break;
    }
}

bool DownloadItem::handleEvent(const SDL_Event& event) {
    if (!m_visible) return false;
    
    // Handle events for child elements
    if (m_filenameLabel->handleEvent(event)) return true;
    if (m_progressBar->handleEvent(event)) return true;
    if (m_speedLabel->handleEvent(event)) return true;
    if (m_statusLabel->handleEvent(event)) return true;
    
    // Handle events for buttons based on download state
    switch (m_download.state) {
        case DownloadState::DOWNLOADING:
            if (m_pauseButton->handleEvent(event)) return true;
            if (m_cancelButton->handleEvent(event)) return true;
            break;
            
        case DownloadState::PAUSED:
            if (m_resumeButton->handleEvent(event)) return true;
            if (m_cancelButton->handleEvent(event)) return true;
            break;
            
        case DownloadState::COMPLETED:
            if (m_openButton->handleEvent(event)) return true;
            break;
            
        default:
            break;
    }
    
    return false;
}

void DownloadItem::update() {
    // Update child elements
    m_filenameLabel->update();
    m_progressBar->update();
    m_speedLabel->update();
    m_statusLabel->update();
    
    // Update buttons
    switch (m_download.state) {
        case DownloadState::DOWNLOADING:
            m_pauseButton->update();
            m_cancelButton->update();
            break;
            
        case DownloadState::PAUSED:
            m_resumeButton->update();
            m_cancelButton->update();
            break;
            
        case DownloadState::COMPLETED:
            m_openButton->update();
            break;
            
        default:
            break;
    }
}

void DownloadItem::setDownload(const DownloadInfo& download) {
    m_download = download;
    updateDisplay();
}

void DownloadItem::setOnPause(std::function<void(const std::string&)> callback) {
    m_onPause = callback;
}

void DownloadItem::setOnResume(std::function<void(const std::string&)> callback) {
    m_onResume = callback;
}

void DownloadItem::setOnCancel(std::function<void(const std::string&)> callback) {
    m_onCancel = callback;
}

void DownloadItem::setOnOpen(std::function<void(const std::string&)> callback) {
    m_onOpen = callback;
}

void DownloadItem::updateDisplay() {
    // Update progress bar
    m_progressBar->setProgress(m_download.progress);
    
    // Update speed label
    std::string speedText = Utils::formatSpeed(m_download.speed);
    m_speedLabel->setText(speedText);
    
    // Update status label
    std::string statusText;
    switch (m_download.state) {
        case DownloadState::QUEUED: statusText = "Queued"; break;
        case DownloadState::DOWNLOADING: statusText = "Downloading"; break;
        case DownloadState::PAUSED: statusText = "Paused"; break;
        case DownloadState::COMPLETED: statusText = "Completed"; break;
        case DownloadState::FAILED: statusText = "Failed"; break;
        case DownloadState::CANCELLED: statusText = "Cancelled"; break;
    }
    m_statusLabel->setText(statusText);
}

} // namespace ZepraCore 