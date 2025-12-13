# ZepraBrowser - Complete File Tree
# Generated: 2025-12-08
# DO NOT TOUCH: source/zepraScript/

zeprabrowser/
├── CMakeLists.txt                              # ROOT BUILD (TODO)
├── Architecture.md                             # This architecture doc
├── README.md
│
├── source/                                     # === NEW ARCHITECTURE ===
│   │
│   ├── zepraScript/                            # ⚠️ DO NOT MODIFY
│   │   ├── include/zeprascript/               # 70+ headers
│   │   │   ├── frontend/ (lexer, parser, ast)
│   │   │   ├── bytecode/ (compiler, opcodes)
│   │   │   ├── runtime/ (vm, value, object)
│   │   │   ├── gc/ (heap, garbage collector)
│   │   │   ├── jit/ (baseline, osr)
│   │   │   ├── builtins/ (Date, Math, Number)
│   │   │   ├── browser/ (Window, DOM bindings)
│   │   │   ├── memory/ (allocators)
│   │   │   ├── debug/ (devtools, inspector)
│   │   │   └── api/ (context, handles)
│   │   ├── src/                               # 50+ implementations
│   │   └── tests/                             # Unit + integration
│   │
│   ├── webCore/                               # Rendering Engine
│   │   ├── include/webcore/
│   │   │   ├── dom.hpp                        # DOM implementation
│   │   │   ├── html_parser.hpp
│   │   │   ├── css_parser.hpp
│   │   │   ├── render_tree.hpp
│   │   │   ├── paint_context.hpp
│   │   │   └── layout_engine.hpp
│   │   ├── src/
│   │   │   ├── dom.cpp
│   │   │   ├── render_tree.cpp
│   │   │   └── paint_context.cpp
│   │   └── CMakeLists.txt
│   │
│   ├── zepraEngine/                           # Browser Window/Shell
│   │   ├── include/engine/
│   │   │   └── browser_window.hpp
│   │   ├── src/
│   │   │   └── zepra_window_demo.cpp          # SDL2 browser UI
│   │   └── CMakeLists.txt
│   │
│   ├── webGpu/                                # GPU Acceleration (TODO)
│   │
│   └── bin/                                   # Built executables
│       └── zepra-browser
│
├── include/                                   # === BROWSER HEADERS ===
│   ├── auth/
│   │   └── zepra_auth.h                       # Ketivee SSO
│   ├── common/
│   │   ├── constants.h
│   │   └── types.h
│   ├── config/
│   │   └── dual_config.h
│   ├── core/
│   │   └── zepra_core.h
│   ├── engine/
│   │   ├── webkit_engine.h                    # WebKit wrapper
│   │   ├── dev_tools.h
│   │   ├── gpu_manager.h
│   │   ├── html_parser.h
│   │   ├── video_player.h
│   │   ├── download_manager.h
│   │   ├── extension.h
│   │   ├── ai_engine.h
│   │   └── ... (15 files)
│   ├── net/
│   │   ├── cookie_manager.h
│   │   └── http.h
│   ├── sandbox/
│   │   └── sandbox_manager.h
│   ├── search/
│   │   └── ketivee_search.h
│   └── ui/
│       ├── window.h
│       ├── tab_manager.h
│       ├── settings_ui.h
│       ├── dev_tools_ui.h
│       ├── auth_ui.h
│       └── extension_manager_ui.h
│
├── src/                                       # === BROWSER SOURCES ===
│   ├── main.cpp                               # Entry point (819 lines)
│   ├── main_unified.cpp
│   │
│   ├── auth/
│   │   └── zepra_auth.cpp                     # 644 lines - CURL/OpenSSL
│   │
│   ├── config/
│   │   ├── config_manager.cpp
│   │   ├── config_manager.h
│   │   └── config_test.cpp
│   │
│   ├── core/
│   │   ├── zepra_core.cpp
│   │   └── ui_elements.cpp
│   │
│   ├── engine/
│   │   ├── webkit_engine.cpp                  # 771 lines
│   │   ├── browser_connector.cpp
│   │   ├── dev_tools.cpp
│   │   ├── download_manager.cpp
│   │   ├── extension_registry.cpp
│   │   ├── gpu_manager.cpp
│   │   ├── html_parser.cpp
│   │   ├── json_bridge.cpp
│   │   ├── video_player.cpp
│   │   └── CMakeLists.txt
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
│   └── ui/
│       ├── window.cpp                         # 878 lines - SDL2/OpenGL
│       ├── tab_manager.cpp
│       ├── settings_ui.cpp
│       ├── dev_tools_ui.cpp
│       └── extension_manager_ui.cpp
│
├── config/                                    # Configuration files
├── docs/                                      # Documentation
├── tools/                                     # Dev tools
└── zepra.ketivee.com/                         # Branding assets

# ============================================
# FILE COUNT SUMMARY
# ============================================
# source/zepraScript/  : 392 files (JS Engine)
# source/webCore/      : 10 files  (Rendering)
# source/zepraEngine/  : 31 files  (Window)
# include/             : 30 files  (Headers)
# src/                 : 25 files  (Browser)
# -------------------------------------------
# TOTAL                : ~490 files
# ============================================
