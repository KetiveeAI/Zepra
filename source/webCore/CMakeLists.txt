cmake_minimum_required(VERSION 3.16)
project(WebCore VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# ZepraScript include path
set(ZEPRASCRIPT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../zepraScript)
include_directories(${ZEPRASCRIPT_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../networking/include)

# ThirdParty includes (nanosvg)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty)

# Source files
set(WEBCORE_SOURCES
    src/dom.cpp
    src/render_tree.cpp
    src/paint_context.cpp
    src/js_dom_bridge.cpp
    src/css_parser.cpp
    src/layout_engine.cpp
    src/html_parser.cpp
    src/browser_ui.cpp
    src/page.cpp
    src/event.cpp
    src/event_target.cpp
    src/scrollbar.cpp
    src/resource_loader.cpp
    src/script_context.cpp
    src/context_menu.cpp
    src/gl_render_backend.cpp
    src/script_loader.cpp
    src/js_realm.cpp
    src/gc_bridge.cpp
    src/sdl_gl_context.cpp
    src/webgl_context.cpp
    src/simple_font.cpp
    src/shortcut_manager.cpp
    src/webgl_bindings.cpp
    src/sidebar.cpp
    src/svg_loader.cpp
    src/navigation_history.cpp
    # HTML Elements (per Architecture.md)
    src/html/html_element.cpp
    src/html/html_dialog_element.cpp
    src/html/html_anchor_element.cpp
    src/html/html_div_element.cpp
    src/html/html_span_element.cpp
    src/html/html_image_element.cpp
    # Form Elements
    src/html/html_form_element.cpp
    src/html/html_input_element.cpp
    src/html/html_button_element.cpp
    # Media Elements
    src/html/html_media_element.cpp
    # Canvas Element
    src/html/html_canvas_element.cpp
    # Browser Interfaces
    src/browser/window.cpp
    src/browser/navigator.cpp
    src/browser/location.cpp
    # CSS Engine
    src/css/css_style_sheet.cpp
    src/css/css_style_declaration.cpp
    src/css/css_rule.cpp
    src/css/css_selector.cpp
    src/css/css_computed_style.cpp
    src/css/css_engine.cpp
    # Compositor & Page Renderer
    src/compositor.cpp
    src/page_renderer.cpp
)

# Find OpenGL for GPU rendering
find_package(OpenGL QUIET)

# Library target
add_library(webcore STATIC ${WEBCORE_SOURCES})

target_include_directories(webcore PUBLIC
    include
    ${CMAKE_CURRENT_SOURCE_DIR}/../zepraScript/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../networking/include
)

# Link with ZepraScript (if available)
if(TARGET zepra-core)
    target_link_libraries(webcore PUBLIC zepra-core)
endif()

# Link with Networking
if(TARGET networking)
    target_link_libraries(webcore PUBLIC networking)
endif()

if(OpenGL_FOUND)
    target_link_libraries(webcore PUBLIC OpenGL::GL)
    target_compile_definitions(webcore PUBLIC ZEPRA_HAS_OPENGL)
endif()

# Link SDL2 Image and TTF
if(SDL2_IMAGE_FOUND)
    target_link_libraries(webcore PUBLIC ${SDL2_IMAGE_LIBRARIES})
    target_include_directories(webcore PUBLIC ${SDL2_IMAGE_INCLUDE_DIRS})
endif()

if(SDL2_TTF_FOUND)
    target_link_libraries(webcore PUBLIC ${SDL2_TTF_LIBRARIES})
    target_include_directories(webcore PUBLIC ${SDL2_TTF_INCLUDE_DIRS})
endif()

# Export settings
set_target_properties(webcore PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

