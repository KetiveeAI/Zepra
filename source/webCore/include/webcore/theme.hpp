/**
 * @file theme.hpp
 * @brief Zepra Browser theme and color definitions
 * 
 * Colors based on Figma RipkaAI design palette
 */

#pragma once

#include "render_tree.hpp"

namespace Zepra::WebCore {

/**
 * @brief Zepra/Ketivee brand colors (from Figma design)
 */
namespace Theme {

// Primary brand colors (from Figma)
constexpr Color primaryDark() { return {67, 0, 177, 255}; }      // #3c1e6dff - Deep purple
constexpr Color primary() { return {159, 100, 255, 255}; }       // #9F64FF - Bright purple
constexpr Color primaryLight() { return {159, 125, 214, 255}; }  // #9F7DD6 - Light purple

// Secondary colors
constexpr Color secondary() { return {193, 163, 214, 255}; }     // #C1A3D6 - Soft purple (65%)
constexpr Color secondaryLight() { return {218, 202, 246, 255}; }// #DACAF6 - Cream pink
constexpr Color accent() { return {220, 164, 164, 255}; }        // #DCA4A4 - Light pink (65%)

// Background colors
constexpr Color background() { return {224, 219, 239, 255}; }    // #E0DBEF - Light lavender
constexpr Color backgroundDark() { return {193, 163, 214, 255}; }// #C1A3D6 - Darker lavender

// Home page gradient (Figma design - pink → purple → blue)
namespace HomeGradient {
    constexpr Color top() { return {218, 173, 193, 255}; }       // #DAADC1 - Soft pink
    constexpr Color middle() { return {201, 164, 213, 255}; }    // #C9A4D5 - Purple  
    constexpr Color bottom() { return {145, 145, 213, 255}; }    // #9191D5 - Blue-purple
}

// Semantic colors
constexpr Color success() { return {76, 175, 80, 255}; }
constexpr Color warning() { return {255, 152, 0, 255}; }
constexpr Color error() { return {244, 67, 54, 255}; }
constexpr Color info() { return {159, 100, 255, 255}; }

// Glassmorphism effects (frosted glass - modern UI)
namespace Glass {
    // Glass backgrounds (semi-transparent with blur effect simulation)
    constexpr Color light() { return {255, 255, 255, 178}; }       // 70% white
    constexpr Color medium() { return {194, 194, 194, 178}; }      // #C2C2C2 @ 70%
    constexpr Color dark() { return {100, 100, 120, 150}; }        // Dark glass
    constexpr Color accent() { return {156, 123, 255, 140}; }      // Purple glass #9C7BFF
    
    // Glass borders (subtle white/light borders)
    constexpr Color border() { return {255, 255, 255, 80}; }       // Subtle white
    constexpr Color borderLight() { return {255, 255, 255, 120}; }
    constexpr Color borderDark() { return {0, 0, 0, 30}; }         // Subtle shadow
    
    // Glass text
    constexpr Color textPrimary() { return {255, 255, 255, 230}; } // White text
    constexpr Color textSecondary() { return {255, 255, 255, 180}; }
    constexpr Color textMuted() { return {255, 255, 255, 128}; }
    
    // Highlight (top edge light for 3D effect)
    constexpr Color highlight() { return {255, 255, 255, 60}; }
}

// Chrome colors (browser UI - Safari-style)
namespace Chrome {
    // Tab bar background
    constexpr Color tabBar() { return {224, 219, 239, 255}; }        // #E0DBEF background
    constexpr Color tabActive() { return {255, 255, 255, 255}; }     // White active tab
    constexpr Color tabInactive() { return {193, 163, 214, 165}; }   // #C1A3D6 @ 65%
    constexpr Color tabHover() { return {218, 202, 246, 255}; }      // #DACAF6 hover
    constexpr Color tabText() { return {67, 0, 177, 255}; }          // #4300B1 text
    constexpr Color tabTextInactive() { return {100, 80, 130, 255}; }// Muted purple
    
    // Navigation bar (integrated with tabs)
    constexpr Color navBar() { return {224, 219, 239, 255}; }        // Same as tabBar
    constexpr Color navBarBorder() { return {193, 163, 214, 255}; }
    
    // Address/URL bar
    constexpr Color addressBar() { return {255, 255, 255, 255}; }
    constexpr Color addressBarBorder() { return {193, 163, 214, 255}; }
    constexpr Color addressBarFocus() { return {159, 100, 255, 255}; }// #9F64FF
    
    // Buttons
    constexpr Color buttonDefault() { return {193, 163, 214, 200}; }
    constexpr Color buttonHover() { return {159, 100, 255, 255}; }
    constexpr Color buttonPressed() { return {67, 0, 177, 255}; }
    constexpr Color buttonDisabled() { return {200, 200, 210, 255}; }
    
    // Close button
    constexpr Color closeButton() { return {100, 80, 130, 255}; }
    constexpr Color closeButtonHover() { return {67, 0, 177, 255}; }
}

// Content colors
namespace Content {
    constexpr Color background() { return {255, 255, 255, 255}; }
    constexpr Color surface() { return {250, 249, 253, 255}; }
    constexpr Color surfaceAlt() { return {245, 243, 250, 255}; }
    
    constexpr Color textPrimary() { return {40, 30, 60, 255}; }
    constexpr Color textSecondary() { return {100, 80, 130, 255}; }
    constexpr Color textMuted() { return {150, 140, 170, 255}; }
    
    constexpr Color border() { return {193, 163, 214, 255}; }
    constexpr Color borderDark() { return {159, 125, 214, 255}; }
    
    constexpr Color link() { return {67, 0, 177, 255}; }
    constexpr Color linkVisited() { return {100, 60, 150, 255}; }
    constexpr Color linkHover() { return {159, 100, 255, 255}; }
}

// Dark mode colors
namespace Dark {
    constexpr Color background() { return {30, 20, 45, 255}; }
    constexpr Color surface() { return {50, 35, 70, 255}; }
    constexpr Color surfaceAlt() { return {70, 50, 95, 255}; }
    
    constexpr Color textPrimary() { return {248, 250, 252, 255}; }
    constexpr Color textSecondary() { return {200, 190, 220, 255}; }
    constexpr Color textMuted() { return {150, 140, 170, 255}; }
    
    constexpr Color border() { return {100, 80, 130, 255}; }
    
    constexpr Color tabBar() { return {40, 28, 60, 255}; }
    constexpr Color navBar() { return {50, 35, 70, 255}; }
}

} // namespace Theme

/**
 * @brief Theme mode
 */
enum class ThemeMode {
    Light,
    Dark,
    System  // Follow system preference
};

/**
 * @brief Theme manager for applying consistent styling
 */
class ThemeManager {
public:
    static ThemeManager& instance();
    
    void setMode(ThemeMode mode) { mode_ = mode; }
    ThemeMode mode() const { return mode_; }
    
    // Get colors based on current mode
    Color background() const;
    Color surface() const;
    Color textPrimary() const;
    Color textSecondary() const;
    Color border() const;
    Color primary() const;
    Color accent() const;
    
private:
    ThemeManager() = default;
    ThemeMode mode_ = ThemeMode::Light;
};

} // namespace Zepra::WebCore
