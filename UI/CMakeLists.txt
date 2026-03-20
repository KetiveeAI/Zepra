cmake_minimum_required(VERSION 3.16)

# Zepra Browser UI — Chrome layer
# Self-contained UI components, no engine dependency internals

set(ZEPRA_UI_SOURCES
    # Browser chrome
    browser/window.cpp
    browser/tab_manager.cpp
    browser/navbar.cpp
    browser/sidebar.cpp
    browser/searchbox.cpp
    browser/dropdown.cpp
    browser/context_menu.cpp

    # Settings & Auth
    settings/settings_ui.cpp
    settings/auth_ui.cpp
    settings/extension_manager_ui.cpp
    settings/privacy_settings_ui.cpp

    # DevTools
    devtools/dev_tools_ui.cpp

    # Panels
    panels/ai_chat_window.cpp
    panels/zepra_webview_panel.cpp
)

# NOTE: This is a source list, not a library target yet.
# The main executable in root CMakeLists.txt references these files
# relative to ${CMAKE_SOURCE_DIR}/UI/
# When UI grows large enough, we can turn this into a static library:
# add_library(zepra-ui STATIC ${ZEPRA_UI_SOURCES})
