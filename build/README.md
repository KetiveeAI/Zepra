# 🦓 Zepra Browser

**Smooth, fast, and wild like a zebra in the tech jungle** ⚡

A lightweight, high-performance web browser built with C++ and SDL2, featuring the integrated **KetiveeSearch** engine.

## 🌟 Features

- **Lightning Fast**: Optimized C++ engine with minimal memory footprint
- **Cross-Platform**: Windows, Linux, and macOS support
- **Custom Search**: Integrated KetiveeSearch engine
- **Modern UI**: Clean, responsive interface with dark/light themes
- **Tab Management**: Multi-tab browsing with efficient memory management
- **Privacy Focused**: Built-in ad blocking and privacy controls
- **Extensible**: Plugin system for custom functionality

## 🏗️ Architecture

```
Zepra Browser Engine
├── Core Engine (C++)
│   ├── HTML Parser
│   ├── CSS Engine
│   ├── JavaScript Runtime (QuickJS)
│   └── Rendering Engine (SDL2/OpenGL)
├── UI Layer
│   ├── Window Management
│   ├── Tab Manager
│   └── Input Handling
├── Network Layer
│   ├── HTTP Client
│   ├── Cookie Management
│   └── Security Layer
└── Search Integration
    └── KetiveeSearch Engine
```

## 🚀 Quick Start

### Prerequisites

- **Windows**: Visual Studio 2019+ or MinGW-w64
- **Linux**: GCC 9+ or Clang 10+
- **macOS**: Xcode 12+ or Clang 10+
- **SDL2**: Cross-platform multimedia library
- **CMake**: Build system

### Building from Source

```bash
# Clone the repository
git clone https://github.com/ketivee/zepra-browser.git
cd zepra-browser

# Create build directory
mkdir build && cd build

# Configure and build
cmake ..
make -j$(nproc)

# Run Zepra
./zepra
```

### Windows Build

```powershell
# Using Visual Studio
cmake -G "Visual Studio 16 2019" -A x64 ..
cmake --build . --config Release

# Using MinGW
cmake -G "MinGW Makefiles" ..
mingw32-make
```

## 🔍 KetiveeSearch Integration

Zepra comes with **KetiveeSearch** as the default search engine, providing:

- **Fast Results**: Optimized search algorithms
- **Privacy**: No tracking or data collection
- **Customizable**: Easy to switch between search engines
- **Local Search**: Offline search capabilities

## 🎨 UI Features

- **Modern Interface**: Clean, minimalist design
- **Dark/Light Themes**: Automatic theme switching
- **Tab Management**: Drag-and-drop tabs, tab groups
- **Bookmarks**: Quick access to favorite sites
- **History**: Smart browsing history with search
- **Downloads**: Integrated download manager

## 🔧 Development

### Project Structure

```
zepra/
├── src/
│   ├── main.cpp              # Application entry point
│   ├── engine/               # Core browser engine
│   │   ├── html_parser.cpp   # HTML parsing and DOM
│   │   ├── css_engine.cpp    # CSS parsing and styling
│   │   ├── renderer.cpp      # Rendering engine
│   │   └── js_runtime.cpp    # JavaScript execution
│   ├── ui/                   # User interface
│   │   ├── window.cpp        # Window management
│   │   ├── tab_manager.cpp   # Tab handling
│   │   └── input.cpp         # Input processing
│   ├── net/                  # Network layer
│   │   ├── http.cpp          # HTTP client
│   │   └── cookies.cpp       # Cookie management
│   └── search/               # Search functionality
│       └── ketivee_search.cpp
├── assets/                   # Resources
│   ├── icons/               # Browser icons
│   └── themes/              # UI themes
├── tests/                   # Test suite
├── docs/                    # Documentation
└── build/                   # Build artifacts
```

### Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## 📄 License

This project is licensed under the **Ketivee App Developer License v1.0** - see [LICENSE_DEV.txt](LICENSE_DEV.txt) for details.

## 🤝 Support

- **Website**: https://ketivee.org
- **Documentation**: [docs/](docs/)
- **Issues**: [GitHub Issues](https://github.com/ketivee/zepra-browser/issues)
- **Discussions**: [GitHub Discussions](https://github.com/ketivee/zepra-browser/discussions)

## 🏆 Acknowledgments

- **Ketivee Team** - For the amazing platform and support
- **SDL2 Community** - For the excellent cross-platform library
- **QuickJS** - For the lightweight JavaScript engine
- **Open Source Community** - For inspiration and contributions

---

**Built with ❤️ by the Ketivee Team**

*Zepra - Where speed meets elegance* 🦓⚡ 