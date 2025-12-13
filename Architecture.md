# ZepraBrowser - Complete Architecture

**Integration with Existing Project Structure**

## Current Project Location
```
~/Documents/zeprabrowser/
```

## 🎯 Architecture Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                      ZepraBrowser Application                    │
│                                                                  │
│  ┌────────────┐  ┌────────────┐  ┌────────────┐  ┌──────────┐ │
│  │ Tabs/UI    │  │ DevTools   │  │ Extensions │  │ Settings │ │
│  │ Manager    │  │ Native UI  │  │ Manager    │  │ Panel    │ │
│  └─────┬──────┘  └─────┬──────┘  └─────┬──────┘  └────┬─────┘ │
│        │               │               │              │        │
└────────┼───────────────┼───────────────┼──────────────┼────────┘
         │               │               │              │
         └───────────────┴───────────────┴──────────────┘
                         │
         ┌───────────────▼────────────────────┐
         │      Browser Engine (Zepra)        │
         │  • Rendering Engine                │
         │  • JavaScript Engine (ZebraScript) │
         │  • Networking Stack                │
         │  • Storage/Cache                   │
         └───────────────┬────────────────────┘
                         │
         ┌───────────────▼────────────────────┐
         │         Platform Layer             │
         │  • OS Integration                  │
         │  • GPU Acceleration                │
         │  • Process Sandboxing              │
         └────────────────────────────────────┘
```

## 📁 Integrated Project Structure

```
zeprabrowser/
├── CMakeLists.txt                           # Root build file
├── README.md
├── LICENSE_PUBLIC.txt
│
├── source/                                   # ← Main source directory
│   │
│   ├── zepraScript/                         # ← JavaScript Engine (already defined)
│   │   ├── CMakeLists.txt
│   │   ├── include/zeprascript/             # Engine headers
│   │   ├── src/                             # Engine implementation
│   │   └── tests/                           # Engine tests
│   │
│   ├── webCore/                             # ← Browser Core (Rendering)
│   │   ├── CMakeLists.txt
│   │   │
│   │   ├── include/
│   │   │   └── webcore/
│   │   │       ├── css/
│   │   │       │   ├── css_parser.hpp
│   │   │       │   ├── css_selector.hpp
│   │   │       │   ├── css_style.hpp
│   │   │       │   ├── css_computed_style.hpp
│   │   │       │   └── css_engine.hpp
│   │   │       │
│   │   │       ├── dom/
│   │   │       │   ├── document.hpp
│   │   │       │   ├── element.hpp
│   │   │       │   ├── node.hpp
│   │   │       │   ├── text_node.hpp
│   │   │       │   ├── comment_node.hpp
│   │   │       │   ├── dom_tree.hpp
│   │   │       │   ├── event.hpp
│   │   │       │   ├── event_target.hpp
│   │   │       │   └── mutation_observer.hpp
│   │   │       │
│   │   │       ├── html/
│   │   │       │   ├── html_parser.hpp
│   │   │       │   ├── html_tokenizer.hpp
│   │   │       │   ├── html_tree_builder.hpp
│   │   │       │   ├── html_element.hpp
│   │   │       │   ├── html_document.hpp
│   │   │       │   └── html_collection.hpp
│   │   │       │
│   │   │       ├── layout/
│   │   │       │   ├── layout_engine.hpp
│   │   │       │   ├── layout_box.hpp
│   │   │       │   ├── block_layout.hpp
│   │   │       │   ├── inline_layout.hpp
│   │   │       │   ├── flex_layout.hpp
│   │   │       │   ├── grid_layout.hpp
│   │   │       │   └── layout_tree.hpp
│   │   │       │
│   │   │       ├── paint/
│   │   │       │   ├── paint_context.hpp
│   │   │       │   ├── paint_layer.hpp
│   │   │       │   ├── painter.hpp
│   │   │       │   ├── compositing.hpp
│   │   │       │   └── display_list.hpp
│   │   │       │
│   │   │       ├── render/
│   │   │       │   ├── render_tree.hpp
│   │   │       │   ├── render_object.hpp
│   │   │       │   ├── render_block.hpp
│   │   │       │   ├── render_inline.hpp
│   │   │       │   ├── render_text.hpp
│   │   │       │   └── render_image.hpp
│   │   │       │
│   │   │       └── core/
│   │   │           ├── page.hpp
│   │   │           ├── frame.hpp
│   │   │           ├── viewport.hpp
│   │   │           └── scrolling.hpp
│   │   │
│   │   └── src/
│   │       ├── css/
│   │       │   ├── css_parser.cpp
│   │       │   ├── css_selector.cpp
│   │       │   ├── css_style.cpp
│   │       │   └── css_engine.cpp
│   │       │
│   │       ├── dom/
│   │       │   ├── document.cpp
│   │       │   ├── element.cpp
│   │       │   ├── node.cpp
│   │       │   ├── dom_tree.cpp
│   │       │   ├── event.cpp
│   │       │   └── event_target.cpp
│   │       │
│   │       ├── html/
│   │       │   ├── html_parser.cpp
│   │       │   ├── html_tokenizer.cpp
│   │       │   ├── html_tree_builder.cpp
│   │       │   └── html_element.cpp
│   │       │
│   │       ├── layout/
│   │       │   ├── layout_engine.cpp
│   │       │   ├── layout_box.cpp
│   │       │   ├── block_layout.cpp
│   │       │   ├── flex_layout.cpp
│   │       │   └── grid_layout.cpp
│   │       │
│   │       ├── paint/
│   │       │   ├── paint_context.cpp
│   │       │   ├── painter.cpp
│   │       │   └── compositing.cpp
│   │       │
│   │       └── render/
│   │           ├── render_tree.cpp
│   │           ├── render_object.cpp
│   │           └── render_block.cpp
│   │
│   ├── webGpu/                              # ← GPU Acceleration
│   │   ├── CMakeLists.txt
│   │   │
│   │   ├── include/
│   │   │   └── webgpu/
│   │   │       ├── gpu_context.hpp
│   │   │       ├── gpu_device.hpp
│   │   │       ├── gpu_pipeline.hpp
│   │   │       ├── gpu_buffer.hpp
│   │   │       ├── gpu_texture.hpp
│   │   │       ├── gpu_shader.hpp
│   │   │       ├── webgl_context.hpp
│   │   │       └── canvas_renderer.hpp
│   │   │
│   │   └── src/
│   │       ├── gpu_context.cpp
│   │       ├── gpu_device.cpp
│   │       ├── gpu_pipeline.cpp
│   │       ├── webgl_context.cpp
│   │       └── canvas_renderer.cpp
│   │
│   ├── zepraEngine/                         # ← Browser Window & UI Integration
│   │   ├── CMakeLists.txt
│   │   │
│   │   ├── include/
│   │   │   └── engine/
│   │   │       ├── browser_window.hpp
│   │   │       ├── tab_manager.hpp
│   │   │       ├── navigation_controller.hpp
│   │   │       ├── history_manager.hpp
│   │   │       ├── bookmark_manager.hpp
│   │   │       ├── download_manager.hpp
│   │   │       └── session_manager.hpp
│   │   │
│   │   └── src/
│   │       ├── browser_window.cpp
│   │       ├── tab_manager.cpp
│   │       ├── navigation_controller.cpp
│   │       ├── history_manager.cpp
│   │       ├── bookmark_manager.cpp
│   │       └── download_manager.cpp
│   │
│   ├── networking/                          # ← NEW: Network Stack
│   │   ├── CMakeLists.txt
│   │   │
│   │   ├── include/
│   │   │   └── networking/
│   │   │       ├── http_client.hpp
│   │   │       ├── http_request.hpp
│   │   │       ├── http_response.hpp
│   │   │       ├── http_cache.hpp
│   │   │       ├── websocket_client.hpp
│   │   │       ├── dns_resolver.hpp
│   │   │       ├── ssl_context.hpp
│   │   │       ├── cookie_manager.hpp
│   │   │       └── resource_loader.hpp
│   │   │
│   │   └── src/
│   │       ├── http_client.cpp
│   │       ├── http_request.cpp
│   │       ├── http_response.cpp
│   │       ├── http_cache.cpp
│   │       ├── websocket_client.cpp
│   │       ├── dns_resolver.cpp
│   │       ├── ssl_context.cpp
│   │       ├── cookie_manager.cpp
│   │       └── resource_loader.cpp
│   │
│   ├── storage/                             # ← NEW: Storage & Cache
│   │   ├── CMakeLists.txt
│   │   │
│   │   ├── include/
│   │   │   └── storage/
│   │   │       ├── local_storage.hpp
│   │   │       ├── session_storage.hpp
│   │   │       ├── indexed_db.hpp
│   │   │       ├── cache_storage.hpp
│   │   │       ├── file_system.hpp
│   │   │       └── quota_manager.hpp
│   │   │
│   │   └── src/
│   │       ├── local_storage.cpp
│   │       ├── session_storage.cpp
│   │       ├── indexed_db.cpp
│   │       ├── cache_storage.cpp
│   │       ├── file_system.cpp
│   │       └── quota_manager.cpp
│   │
│   ├── platform/                            # ← NEW: Platform Integration
│   │   ├── CMakeLists.txt
│   │   │
│   │   ├── include/
│   │   │   └── platform/
│   │   │       ├── window_system.hpp
│   │   │       ├── event_loop.hpp
│   │   │       ├── clipboard.hpp
│   │   │       ├── notification.hpp
│   │   │       ├── file_picker.hpp
│   │   │       ├── system_info.hpp
│   │   │       └── process_manager.hpp
│   │   │
│   │   └── src/
│   │       ├── windows/                     # Windows implementation
│   │       │   ├── window_system_win.cpp
│   │       │   └── clipboard_win.cpp
│   │       │
│   │       ├── linux/                       # Linux implementation
│   │       │   ├── window_system_linux.cpp
│   │       │   └── clipboard_linux.cpp
│   │       │
│   │       ├── macos/                       # macOS implementation
│   │       │   ├── window_system_macos.mm
│   │       │   └── clipboard_macos.mm
│   │       │
│   │       └── common/
│   │           ├── event_loop.cpp
│   │           └── system_info.cpp
│   │
│   ├── sandbox/                             # ← NEW: Process Sandboxing
│   │   ├── CMakeLists.txt
│   │   │
│   │   ├── include/
│   │   │   └── sandbox/
│   │   │       ├── sandbox_manager.hpp
│   │   │       ├── process_launcher.hpp
│   │   │       ├── ipc_channel.hpp
│   │   │       └── security_policy.hpp
│   │   │
│   │   └── src/
│   │       ├── sandbox_manager.cpp
│   │       ├── process_launcher.cpp
│   │       ├── ipc_channel.cpp
│   │       └── security_policy.cpp
│   │
│   ├── extensions/                          # ← NEW: Extension System
│   │   ├── CMakeLists.txt
│   │   │
│   │   ├── include/
│   │   │   └── extensions/
│   │   │       ├── extension_manager.hpp
│   │   │       ├── extension_runtime.hpp
│   │   │       ├── extension_api.hpp
│   │   │       ├── content_script.hpp
│   │   │       └── background_script.hpp
│   │   │
│   │   └── src/
│   │       ├── extension_manager.cpp
│   │       ├── extension_runtime.cpp
│   │       ├── extension_api.cpp
│   │       └── content_script.cpp
│   │
│   ├── media/                               # ← NEW: Media Support
│   │   ├── CMakeLists.txt
│   │   │
│   │   ├── include/
│   │   │   └── media/
│   │   │       ├── video_decoder.hpp
│   │   │       ├── audio_decoder.hpp
│   │   │       ├── media_player.hpp
│   │   │       ├── webrtc_manager.hpp
│   │   │       └── media_source.hpp
│   │   │
│   │   └── src/
│   │       ├── video_decoder.cpp
│   │       ├── audio_decoder.cpp
│   │       ├── media_player.cpp
│   │       └── webrtc_manager.cpp
│   │
│   ├── devtools/                            # ← NEW: Browser DevTools
│   │   ├── CMakeLists.txt
│   │   │
│   │   ├── include/
│   │   │   └── devtools/
│   │   │       ├── devtools_window.hpp
│   │   │       ├── elements_panel.hpp
│   │   │       ├── console_panel.hpp
│   │   │       ├── network_panel.hpp
│   │   │       ├── sources_panel.hpp
│   │   │       ├── performance_panel.hpp
│   │   │       └── memory_panel.hpp
│   │   │
│   │   └── src/
│   │       ├── devtools_window.cpp
│   │       ├── elements_panel.cpp
│   │       ├── console_panel.cpp
│   │       ├── network_panel.cpp
│   │       ├── sources_panel.cpp
│   │       ├── performance_panel.cpp
│   │       └── memory_panel.cpp
│   │
│   └── ThirdParty/                          # ← External dependencies
│       ├── skia/                            # Graphics library
│       ├── freetype/                        # Font rendering
│       ├── harfbuzz/                        # Text shaping
│       ├── libpng/                          # PNG images
│       ├── libjpeg/                         # JPEG images
│       ├── libwebp/                         # WebP images
│       ├── zlib/                            # Compression
│       ├── openssl/                         # SSL/TLS
│       └── sqlite/                          # Database
│
├── include/                                 # ← Existing browser headers
│   ├── auth/
│   │   └── zepra_auth.h
│   │
│   ├── common/
│   │   ├── constants.h
│   │   └── types.h
│   │
│   ├── config/
│   │   └── dual_config.h
│   │
│   ├── core/
│   │   └── zepra_core.h
│   │
│   ├── engine/
│   │   ├── ai_engine.h                     # AI features
│   │   ├── ai_manager.h
│   │   ├── attention_manager.h
│   │   ├── auto_fill_manager.h
│   │   ├── browser_connector.h
│   │   ├── dev_tools.h
│   │   ├── download_manager.h
│   │   ├── extension.h
│   │   ├── gpu_manager.h
│   │   ├── html_parser.h
│   │   ├── html_parser_modern.h
│   │   ├── json_bridge.h
│   │   ├── video_player.h
│   │   ├── web_kernel.h
│   │   └── webkit_engine.h
│   │
│   ├── net/
│   │   ├── cookie_manager.h
│   │   └── http.h
│   │
│   ├── sandbox/
│   │   └── sandbox_manager.h
│   │
│   ├── search/
│   │   └── ketivee_search.h                # Integrated search
│   │
│   └── ui/
│       ├── auth_ui.h
│       ├── dev_tools_ui.h
│       ├── extension_manager_ui.h
│       ├── settings_ui.h
│       ├── tab_manager.h
│       └── window.h
│
├── src/                                     # ← Existing browser sources
│   ├── auth/
│   │   └── zepra_auth.cpp
│   │
│   ├── config/
│   │   ├── config_manager.cpp
│   │   ├── config_manager.h
│   │   └── config_test.cpp
│   │
│   ├── core/
│   │   ├── ui_elements.cpp
│   │   └── zepra_core.cpp
│   │
│   ├── engine/
│   │   ├── browser_connector.cpp
│   │   ├── dev_tools.cpp
│   │   ├── download_manager.cpp
│   │   ├── extension_registry.cpp
│   │   ├── gpu_manager.cpp
│   │   ├── html_parser.cpp
│   │   ├── json_bridge.cpp
│   │   ├── video_player.cpp
│   │   └── webkit_engine.cpp
│   │
│   ├── net/
│   │   └── cookie_manager.cpp
│   │
│   ├── platform/
│   │   └── platform_infrastructure.cpp
│   │
│   ├── sandbox/
│   │   └── sandbox_manager.cpp
│   │
│   ├── search/
│   │   └── ketivee_search.cpp
│   │
│   ├── ui/
│   │   ├── dev_tools_ui.cpp
│   │   ├── extension_manager_ui.cpp
│   │   ├── settings_ui.cpp
│   │   ├── tab_manager.cpp
│   │   └── window.cpp
│   │
│   ├── main.cpp                             # Browser entry point
│   └── main_unified.cpp
│
├── tests/                                   # ← Browser tests
│   ├── CMakeLists.txt
│   │
│   ├── unit/
│   │   ├── dom_tests.cpp
│   │   ├── css_parser_tests.cpp
│   │   ├── layout_tests.cpp
│   │   ├── render_tests.cpp
│   │   └── network_tests.cpp
│   │
│   ├── integration/
│   │   ├── page_load_tests.cpp
│   │   ├── navigation_tests.cpp
│   │   └── extension_tests.cpp
│   │
│   └── web_platform/
│       └── wpt_runner.cpp                  # Web Platform Tests
│
├── tools/                                   # ← Development tools
│   ├── zepracoretest/
│   │   ├── main_orchestrator.py
│   │   ├── analysis_tools/
│   │   ├── analytics/
│   │   ├── build_tools/
│   │   ├── debugging/
│   │   ├── dev_tools/
│   │   ├── performance_tools/
│   │   └── testing_tools/
│   │
│   └── sdk_tools/
│       ├── extension_packager/
│       └── theme_builder/
│
├── resources/                               # ← Browser resources
│   ├── icons/
│   ├── themes/
│   ├── default_pages/
│   │   ├── new_tab.html
│   │   ├── error_page.html
│   │   └── about.html
│   └── translations/
│       ├── en.json
│       └── hi.json
│
├── config/                                  # ← Configuration
│   ├── zepra_config.json
│   ├── default_settings.json
│   └── feature_flags.json
│
├── bin/                                     # ← Output binaries
│   ├── configs/
│   │   ├── system.ncf
│   │   └── zepra_browser.tie
│   └── Debug/
│       └── ZepraBrowser.exe
│
├── docs/                                    # ← Documentation
│   ├── ARCHITECTURE.md
│   ├── API_REFERENCE.md
│   ├── BUILDING.md
│   ├── CONTRIBUTING.md
│   ├── CONFIGURATION_SYSTEM.md
│   ├── DEVELOPMENT_STATUS.md
│   ├── SEARCH_ENGINE_INTEGRATION.md
│   └── ADVANCED_DEVELOPER_TOOLS.md
│
├── external/                                # ← External libraries
│   └── nlohmann/
│       └── json.hpp
│
└── zepra.ketivee.com/                      # ← Branding assets
    ├── color.xml
    ├── logo.svg
    └── zepraicon/
        └── *.svg
```

## 🏗️ Component Architecture

### 1. Browser Core Stack

```
┌────────────────────────────────────────┐
│          Application Layer             │
│  • Tab Management                      │
│  • Window Management                   │
│  • User Interface                      │
│  • Settings & Preferences              │
└──────────────┬─────────────────────────┘
               │
┌──────────────▼─────────────────────────┐
│         Browser Engine Layer           │
│  ┌──────────────┐  ┌────────────────┐ │
│  │  Rendering   │  │  JavaScript    │ │
│  │  Engine      │◄─┤  Engine        │ │
│  │  (WebCore)   │  │  (ZebraScript) │ │
│  └──────┬───────┘  └────────────────┘ │
│         │                              │
│  ┌──────▼───────┐  ┌────────────────┐ │
│  │  Layout      │  │  Networking    │ │
│  │  Engine      │  │  Stack         │ │
│  └──────┬───────┘  └────────────────┘ │
│         │                              │
│  ┌──────▼───────┐  ┌────────────────┐ │
│  │  Paint       │  │  Storage       │ │
│  │  & Graphics  │  │  System        │ │
│  └──────────────┘  └────────────────┘ │
└──────────────┬─────────────────────────┘
               │
┌──────────────▼─────────────────────────┐
│          Platform Layer                │
│  • OS Integration                      │
│  • GPU Acceleration (WebGPU)           │
│  • Process Sandboxing                  │
│  • IPC (Inter-Process Communication)   │
└────────────────────────────────────────┘
```

### 2. Page Loading Pipeline

```
User Types URL
      ↓
Navigation Controller
      ↓
DNS Resolution
      ↓
HTTP Request → HTTP Response
      ↓
HTML Parser → DOM Tree
      ↓
CSS Parser → CSSOM Tree
      ↓
JavaScript Execution (ZebraScript)
      ↓
DOM + CSSOM → Render Tree
      ↓
Layout Engine → Layout Tree
      ↓
Paint Engine → Display List
      ↓
Compositing → GPU Layers
      ↓
Screen Display
```

### 3. Multi-Process Architecture

```
┌─────────────────────────────────────────────────────────┐
│                   Browser Process                        │
│  • Main UI                                              │
│  • Tab Management                                       │
│  • Network Service                                      │
│  • Storage Service                                      │
└───┬─────────────────┬─────────────────┬────────────────┘
    │                 │                 │
    │ IPC             │ IPC             │ IPC
    │                 │                 │
┌───▼──────────┐  ┌───▼──────────┐  ┌──▼──────────────┐
│  Renderer    │  │  Renderer    │  │  GPU Process    │
│  Process 1   │  │  Process 2   │  │                 │
│  (Tab 1)     │  │  (Tab 2)     │  │  • Compositing  │
│              │  │              │  │  • Rendering    │
│  • WebCore   │  │  • WebCore   │  │  • 3D Graphics  │
│  • JS Engine │  │  • JS Engine │  │                 │
└──────────────┘  └──────────────┘  └─────────────────┘
```

## 🔗 Integration Points

### ZebraScript ↔ WebCore Integration

```cpp
// source/webCore/src/dom/document.cpp
#include "zeprascript/runtime/vm.hpp"
#include "zeprascript/browser/window.hpp"

namespace WebCore {

class Document {
private:
    // JavaScript VM instance
    std::unique_ptr<Zepra::Runtime::VM> js_vm_;
    
public:
    void initializeJavaScript() {
        js_vm_ = Zepra::Runtime::VM::create();
        
        // Expose DOM to JavaScript
        auto* window = js_vm_->getGlobalObject();
        window->setProperty("document", this);
    }
    
    void executeScript(const std::string& code) {
        js_vm_->execute(code);
    }
};

} // namespace WebCore
```

### WebCore ↔ Platform Integration

```cpp
// source/platform/include/platform/window_system.hpp
namespace Platform {

class WindowSystem {
public:
    virtual void createWindow() = 0;
    virtual void destroyWindow() = 0;
    virtual void setTitle(const std::string& title) = 0;
    virtual void resize(int width, int height) = 0;
};

// Platform-specific implementations
#ifdef _WIN32
class WindowSystemWin : public WindowSystem { /* ... */ };
#elif defined(__linux__)
class WindowSystemLinux : public WindowSystem { /* ... */ };
#elif defined(__APPLE__)
class WindowSystemMacOS : public WindowSystem { /* ... */ };
#endif

} // namespace Platform
```

## 📊 Root CMakeLists.txt Integration

```cmake
cmake_minimum_required(VERSION 3.20)
project(ZepraBrowser VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(ZEPRA_BUILD_TESTS "Build tests" ON)
option(ZEPRA_BUILD_DEVTOOLS "Build DevTools" ON)
option(ZEPRA_ENABLE_GPU "Enable GPU acceleration" ON)
option(ZEPRA_ENABLE_EXTENSIONS "Enable extension support" ON)

# Browser Components (in source/)
add_subdirectory(source/zepraScript)      # JavaScript Engine
add_subdirectory(source/webCore)          # Rendering Engine
add_subdirectory(source/webGpu)           # GPU Acceleration
add_subdirectory(source/zepraEngine)      # Browser Window/UI
add_subdirectory(source/networking)       # Network Stack
add_subdirectory(source/storage)          # Storage System
add_subdirectory(source/platform)         # Platform Layer
add_subdirectory(source/sandbox)          # Sandboxing
add_subdirectory(source/extensions)       # Extensions
add_subdirectory(source/media)            # Media Support
add_subdirectory(source/devtools)         # DevTools

# Legacy Components (in src/ and include/)
add_subdirectory(src/auth)
add_subdirectory(src/config)
add_subdirectory(src/search)

# Main Browser Application
add_executable(ZepraBrowser
    src/main.cpp
    src/core/zepra_core.cpp
    src/ui/window.cpp
    src/ui/tab_manager.cpp
    # ... other sources
)

target_link_libraries(ZepraBrowser
    PRIVATE
        zepra-script          # JavaScript Engine
        web-core              # Rendering Engine
        zepra-engine          # Browser Window
        networking            # HTTP/WebSocket
        storage               # LocalStorage/IndexedDB
        platform              # OS Integration
        sandbox               # Security
        extensions            # Extension System
        media                 # Audio/Video
        devtools              # Developer Tools
)

# Tests
if(ZEPRA_BUILD_TESTS)
    add_subdirectory(tests)
endif()
```

---

## 🔧 Build Instructions

### Prerequisites
```bash
# Ubuntu/Debian
sudo apt install cmake ninja-build libsdl2-dev libcurl4-openssl-dev \
    libjson-c-dev libssl-dev libsqlite3-dev

# macOS
brew install cmake ninja sdl2 curl openssl sqlite
```

### Build Commands
```bash
# Configure
cmake -B build -G Ninja \
    -DZEPRA_BUILD_TESTS=ON \
    -DZEPRA_ENABLE_GPU=ON

# Build
cmake --build build -j$(nproc)

# Run
./build/bin/ZepraBrowser
```

### Component-Only Builds
```bash
# Build only ZepraScript
cmake --build build --target zepra-script

# Build only Browser Window Demo
cmake --build build --target zepra-browser
```

---

## 📊 File Summary

| Component | Location | Files | Status |
|-----------|----------|-------|--------|
| **ZepraScript** | `source/zepraScript/` | 392 | ✅ Complete |
| **WebCore** | `source/webCore/` | 10 | ✅ Implemented |
| **ZepraEngine** | `source/zepraEngine/` | 31 | ✅ Implemented |
| **Browser Headers** | `include/` | 30 | ✅ Complete |
| **Browser Sources** | `src/` | 25 | ✅ Complete |
| **Tests** | `tests/` | 15 | 🔄 In Progress |
| **TOTAL** | | **~500** | ✅ |

---

## 🚀 Current Status

### ✅ Completed
- ZepraScript JavaScript Engine (full implementation)
- WebCore Rendering Engine (DOM, CSS, Layout)
- Browser Window Demo (SDL2)
- DevTools Protocol (Chrome CDP)
- Authentication System (Ketivee SSO)

### 🔄 In Progress
- GPU Acceleration (WebGPU)
- Extension System
- Media Support

### 📋 Planned
- Service Workers
- WebRTC
- PWA Support

---

## 📝 Notes

1. **DO NOT MODIFY** `source/zepraScript/` without careful consideration
2. Browser tests require ZepraScript to be built first
3. Use `FILETREE.md` for quick reference of all files
4. See `docs/` for detailed API documentation

---

**Last Updated:** 2025-12-08
**Version:** 1.0.0
**Maintainer:** Swanaya