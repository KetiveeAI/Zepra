cmake_minimum_required(VERSION 3.28) # Dont change this
project(nxrender VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
set(NXRENDER_SOURCES
    # nxgfx
    nxgfx/color.cpp
    nxgfx/context.cpp
    nxgfx/context3d.cpp
    nxgfx/text.cpp
    
    # core
    core/compositor.cpp
    core/surface.cpp
    
    # widgets
    widgets/widget.cpp
    widgets/button.cpp
    widgets/label.cpp
    widgets/textfield.cpp
    widgets/container.cpp
    widgets/scrollview.cpp
    widgets/listview.cpp
    widgets/tabview.cpp
    
    # layout
    layout/flexbox.cpp
    layout/block_layout.cpp
    layout/inline_layout.cpp
    layout/positioned_layout.cpp
    layout/grid.cpp
    
    # theme
    theme/theme.cpp
    
    # input
    input/mouse.cpp
    input/keyboard.cpp
    
    # animation
    animation/animator.cpp
    
    # platform
    platform/linux_x11.cpp
    
    # web layer
    web/dom_renderer.cpp
    web/style_resolver.cpp
    
    # bridge to libnxrender.a (C library) - DISABLED: requires external nxgfx C lib
    # nxrender_c_bridge.cpp
    
    # main
    nxrender.cpp
)

# PDF Backend Configuration
option(NXPDF_USE_BUILTIN "Use built-in nxpdf library (ships with Zepra)" ON)
option(NXPDF_USE_OS "Use NeolyxOS syscall/IPC for PDF rendering" OFF)

if(NXPDF_USE_BUILTIN)
    list(APPEND NXRENDER_SOURCES
        nxpdf/nx_pdf.cpp
        nxpdf/parser/nx_pdf_lexer.cpp
        nxpdf/parser/nx_pdf_xref.cpp
        nxpdf/parser/nx_pdf_objects.cpp
        nxpdf/parser/nx_pdf_document.cpp
        nxpdf/renderer/nx_pdf_page.cpp
        nxpdf/renderer/nx_pdf_content.cpp
        nxpdf/renderer/nx_pdf_graphics.cpp
        nxpdf/renderer/nx_pdf_text.cpp
        nxpdf/fonts/nx_pdf_font.cpp
        nxpdf/fonts/nx_pdf_tounicode.cpp
        nxpdf/images/nx_pdf_image.cpp
        nxpdf/editor/nx_pdf_edit.cpp
        nxpdf/editor/nx_pdf_annot.cpp
        nxpdf/editor/nx_pdf_writer.cpp
    )
endif()

# Create library
add_library(nxrender STATIC ${NXRENDER_SOURCES})

# Include directories
target_include_directories(nxrender PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/nxpdf
)

if(NXPDF_USE_BUILTIN)
    target_compile_definitions(nxrender PUBLIC NXPDF_USE_BUILTIN)
elseif(NXPDF_USE_OS)
    target_compile_definitions(nxrender PUBLIC NXPDF_USE_OS)
endif()

# Include C library headers for bridge (from nxrender_c)
target_include_directories(nxrender PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../gui/nxrender_c/include
)

# Link dependencies
find_package(OpenGL REQUIRED)
find_package(Freetype REQUIRED)

target_link_libraries(nxrender PUBLIC
    OpenGL::GL
    Freetype::Freetype
)

# Platform-specific
if(UNIX AND NOT APPLE)
    target_link_libraries(nxrender PUBLIC X11)
endif()

# Install
install(TARGETS nxrender
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(DIRECTORY animation core input layout nxgfx platform theme web widgets
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

# Examples (optional)
option(BUILD_EXAMPLES "Build example applications" OFF)
if(BUILD_EXAMPLES)
    add_executable(browser_ui_example examples/browser_ui_example.cpp)
    target_link_libraries(browser_ui_example PRIVATE nxrender)
endif()
