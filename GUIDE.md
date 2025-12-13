# ZepraBrowser Development Guide

**Project:** ZepraBrowser - Independent Web Browser  
**Engine:** ZepraScript (JavaScript) + WebCore (HTML/CSS) + NXRender (Graphics)  
**Trademark:** ZepraScript is a trademark of KetiveeAI  
**Last Updated:** 2025-12-13

---

## Architecture Overview

```
+----------------------------------------------------------+
|                    ZepraBrowser                          |
+----------------------------------------------------------+
|  UI Layer (NXRENDER)                                     |
|  - Tab Bar, Address Bar, DevTools                        |
|  - Native X11/Wayland + OpenGL rendering                 |
+----------------------------------------------------------+
|  Browser Core                                            |
|  - Tab Management, History, Bookmarks                    |
|  - Downloads, Settings, Extensions                       |
+----------------------------------------------------------+
|  WebCore Engine                                          |
|  - HTML Parser (Tokenizer + Tree Builder)                |
|  - CSS Engine (Parser + Selector Matching)               |
|  - Layout Engine (Box Model + Flow)                      |
|  - Paint/Composite (GPU accelerated)                     |
+----------------------------------------------------------+
|  ZepraScript Engine                                      |
|  - Lexer, Parser, AST                                    |
|  - Bytecode Compiler                                     |
|  - Multi-tier JIT (Baseline, DFG, FTL)                   |
|  - Garbage Collector                                     |
+----------------------------------------------------------+
|  Networking                                              |
|  - HTTP/HTTPS Client (curl/OpenSSL)                      |
|  - WebSocket, Fetch API                                  |
+----------------------------------------------------------+
|  Platform Layer                                          |
|  - X11/Wayland Window                                    |
|  - File System, Process Sandbox                          |
+----------------------------------------------------------+
```

---

## Directory Structure

```
zeprabrowser/
├── source/
│   ├── zepraScript/        # JavaScript engine (trademark of KetiveeAI)
│   │   ├── src/
│   │   │   ├── frontend/   # Lexer, Parser, AST
│   │   │   ├── compiler/   # Bytecode generation
│   │   │   ├── runtime/    # VM, GC, Objects
│   │   │   ├── builtins/   # Array, String, Math, etc.
│   │   │   ├── jit/        # JIT compilers
│   │   │   └── browser/    # DOM, Fetch, Events APIs
│   │   └── include/
│   │
│   ├── webCore/            # HTML/CSS engine
│   │   ├── src/
│   │   │   ├── html/       # HTML tokenizer, tree builder
│   │   │   ├── css/        # CSS parser, CSSOM
│   │   │   ├── dom/        # DOM implementation
│   │   │   ├── layout/     # Box model, flow layout
│   │   │   └── paint/      # Rendering, compositing
│   │   └── include/
│   │
│   ├── nxrender/           # Native rendering (Rust)
│   │   ├── nxgfx/          # GPU graphics backend
│   │   ├── nxrender-core/  # Compositor, surfaces
│   │   ├── nxrender-widgets/# UI widgets
│   │   ├── nxrender-input/ # Mouse, keyboard, touch
│   │   └── nxrender-ffi/   # C bindings
│   │
│   ├── networking/         # HTTP client
│   │   ├── http_client.cpp
│   │   ├── websocket.cpp
│   │   └── tls.cpp
│   │
│   └── integration/        # ZepraScript <-> DOM bridge
│
├── src/                    # Browser application
│   ├── main_nxrender.cpp   # Main entry (NXRENDER)
│   ├── ui/                 # Tab manager, windows
│   ├── engine/             # WebKit engine wrapper
│   └── auth/               # Authentication
│
├── include/                # Public headers
├── resources/              # Icons, themes, images
└── docs/                   # Documentation
```

---

## Key Components

### 1. ZepraScript Engine

**Location:** `source/zepraScript/`

The JavaScript engine implementing ECMAScript 2024. Features:
- Multi-tier JIT compilation
- Generational garbage collection
- ES modules support
- Web API bindings

**Key Files:**
- `src/frontend/lexer.cpp` - Tokenization
- `src/frontend/parser.cpp` - AST generation
- `src/compiler/bytecode_generator.cpp` - Bytecode
- `src/runtime/vm.cpp` - Virtual machine
- `src/jit/baseline_jit.cpp` - First-tier JIT

### 2. WebCore Engine

**Location:** `source/webCore/`

HTML/CSS parsing and layout engine.

**Key Files:**
- `src/html/tokenizer.cpp` - HTML5 tokenizer
- `src/html/tree_builder.cpp` - DOM construction
- `src/css/parser.cpp` - CSS parsing
- `src/css/selector.cpp` - Selector matching
- `src/layout/box.cpp` - Box model
- `src/layout/block.cpp` - Block layout
- `src/paint/painter.cpp` - Rendering

### 3. NXRENDER Graphics

**Location:** `source/nxrender/`

Rust-based rendering system with C FFI.

**Key Crates:**
- `nxgfx` - GPU context, primitives, text
- `nxrender-core` - Compositor, surfaces
- `nxrender-widgets` - Button, TextField, etc.
- `nxrender-input` - Mouse, keyboard, gestures
- `nxrender-ffi` - C bindings for C++ integration

### 4. Networking

**Location:** `source/networking/`

HTTP client using libcurl and OpenSSL.

**Key Files:**
- `http_client.cpp` - Request/response handling
- `cookie_jar.cpp` - Cookie management
- `tls.cpp` - HTTPS support

---

## Building

### Prerequisites

```bash
# Ubuntu/Debian
sudo apt install build-essential cmake libcurl4-openssl-dev \
    libssl-dev libx11-dev libgl1-mesa-dev libfreetype6-dev \
    libharfbuzz-dev rustc cargo

# Build NXRENDER
cd source/nxrender
cargo build --release

# Build Browser
mkdir build && cd build
cmake .. -DUSE_NXRENDER=ON
make -j$(nproc)
```

### Running

```bash
cd build/bin
LD_LIBRARY_PATH=../../source/nxrender/target/release ./zepra_nxrender
```

---

## Development Workflow

### Adding a Web API

1. Define interface in `source/zepraScript/include/zeprascript/browser/`
2. Implement in `source/zepraScript/src/browser/`
3. Register in global object
4. Add tests

### Adding HTML Element Support

1. Add element handling in `source/webCore/src/html/tree_builder.cpp`
2. Implement DOM interface in `source/webCore/src/dom/`
3. Add layout behavior in `source/webCore/src/layout/`
4. Add rendering in `source/webCore/src/paint/`

### Adding CSS Property

1. Add property to enum in `source/webCore/include/webcore/css/properties.h`
2. Add parsing in `source/webCore/src/css/parser.cpp`
3. Add computation in `source/webCore/src/css/computed_style.cpp`
4. Use in layout/paint

---

## References

### Specifications
- [HTML Living Standard](https://html.spec.whatwg.org/)
- [CSS Specifications](https://www.w3.org/Style/CSS/)
- [ECMAScript 2024](https://tc39.es/ecma262/)
- [DOM Living Standard](https://dom.spec.whatwg.org/)
- [Fetch Standard](https://fetch.spec.whatwg.org/)

### MDN Documentation
- [Web APIs](https://developer.mozilla.org/en-US/docs/Web/API)
- [HTML Elements](https://developer.mozilla.org/en-US/docs/Web/HTML/Element)
- [CSS Reference](https://developer.mozilla.org/en-US/docs/Web/CSS/Reference)
- [JavaScript Reference](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference)

### Browser Engine References
- [WebKit Source](https://github.com/WebKit/WebKit)
- [Servo (Rust)](https://github.com/servo/servo)
- [Ladybird](https://github.com/LadybirdBrowser/ladybird)

---

## Code Style

### C++ (ZepraScript, WebCore)
- C++17 standard
- PascalCase for classes
- camelCase for methods
- snake_case for variables
- Trailing underscore for members

### Rust (NXRENDER)
- Rust 2021 edition
- snake_case everywhere
- Document public APIs
- Use Result for errors

---

## Testing

```bash
# ZepraScript tests
cd source/zepraScript/build
ctest

# WebCore tests
cd source/webCore/build
ctest

# NXRENDER tests
cd source/nxrender
cargo test
```

---

## Trademark Notice

**ZepraScript** is a registered trademark of **KetiveeAI**. All rights reserved.
