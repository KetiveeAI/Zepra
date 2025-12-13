# 🦓 Zepra Core Browser

**Smooth, fast, and wild like a zebra in the tech jungle** ⚡

A lightweight, high-performance web browser built with **C/C++** and SDL2, featuring integrated **Ketivee Authentication System** and **KetiveeSearch** engine.

## 🌟 Features

### 🔐 **Integrated Authentication System**
- **Seamless Login**: Integrated with Ketivee authentication server
- **2FA Support**: Two-factor authentication with TOTP codes
- **Session Management**: Automatic token refresh and session persistence
- **Cross-Domain Auth**: Share authentication across Ketivee subdomains
- **Password Manager**: Secure storage for third-party website credentials
- **Security First**: HTTP-only cookies, secure token handling

### 🚀 **Performance & Speed**
- **Lightning Fast**: Optimized C++ engine with minimal memory footprint
- **Hardware Acceleration**: OpenGL rendering for smooth graphics
- **Memory Efficient**: Smart tab management and resource optimization
- **Cross-Platform**: Windows, Linux, and macOS support

### 🔍 **Search & Navigation**
- **KetiveeSearch**: Integrated search engine with privacy focus
- **Smart Address Bar**: URL completion and search suggestions
- **Tab Management**: Multi-tab browsing with drag-and-drop
- **Bookmarks**: Quick access to favorite sites
- **History**: Smart browsing history with search

### 🛡️ **Security & Privacy**
- **Built-in Ad Blocking**: Automatic ad and tracker blocking
- **HTTPS Enforcement**: Secure connections by default
- **Sandbox Mode**: Isolated browsing for enhanced security
- **Privacy Controls**: Granular privacy settings
- **Content Filtering**: Safe browsing with customizable filters

### 🎨 **Modern UI**
- **Dark/Light Themes**: Automatic theme switching
- **Responsive Design**: Adapts to different screen sizes
- **Customizable**: Extensive UI customization options
- **Accessibility**: Screen reader support and keyboard navigation

## 🏗️ Architecture

```
Zepra Core Browser Engine
├── Core Engine (C++)
│   ├── HTML Parser
│   ├── CSS Engine
│   ├── JavaScript Runtime (QuickJS)
│   └── Rendering Engine (SDL2/OpenGL)
├── Authentication System
│   ├── ZepraAuthManager
│   ├── Session Management
│   ├── 2FA Handler
│   └── Password Manager
├── UI Layer
│   ├── Window Management
│   ├── Tab Manager
│   ├── Auth Dialogs
│   └── Input Handling
├── Network Layer
│   ├── HTTP Client (CURL)
│   ├── Cookie Management
│   └── Security Layer
└── Search Integration
    └── KetiveeSearch Engine
```

## 🚀 Quick Start

### Prerequisites

#### **Ubuntu/Debian**
```bash
sudo apt update
sudo apt install build-essential cmake pkg-config
sudo apt install libsdl2-dev libsdl2-ttf-dev libsdl2-image-dev
sudo apt install libcurl4-openssl-dev libssl-dev libjson-c-dev
```

#### **CentOS/RHEL/Fedora**
```bash
sudo yum groupinstall "Development Tools"
sudo yum install cmake pkg-config
sudo yum install SDL2-devel SDL2_ttf-devel SDL2_image-devel
sudo yum install libcurl-devel openssl-devel json-c-devel
```

#### **macOS**
```bash
brew install cmake pkg-config
brew install sdl2 sdl2_ttf sdl2_image
brew install curl openssl json-c
```

#### **Windows**
- Install Visual Studio 2019+ or MinGW-w64
- Install vcpkg and required packages:
```cmd
vcpkg install sdl2 sdl2-ttf sdl2-image curl openssl json-c
```

### Building from Source

```bash
# Clone the repository
git clone https://github.com/ketivee/zepra-browser.git
cd zepra-browser

# Make build script executable
chmod +x build-zepra-core.sh

# Build the browser
./build-zepra-core.sh

# Run the browser
./run-zepra.sh
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

## 🔐 Authentication System

### **Login Process**
1. **Email/Password**: Enter your Ketivee credentials
2. **2FA Verification**: If enabled, enter TOTP code
3. **Session Creation**: Secure token-based session
4. **Cross-Domain Access**: Automatic authentication for Ketivee services

### **Supported Domains**
- `ketivee.com` - Main platform
- `auth.ketivee.com` - Authentication server
- `ketiveeai.com` - AI platform
- `docs.ketivee.com` - Documentation
- `mail.ketivee.com` - Email service
- `workspace.ketivee.com` - Development workspace
- And more...

### **Security Features**
- **HTTP-Only Cookies**: Secure cookie storage
- **Token Refresh**: Automatic session renewal
- **Domain Validation**: Whitelist-based access control
- **Encrypted Storage**: Secure credential storage
- **Session Timeout**: Configurable session limits

## 🎨 UI Components

### **Authentication Dialogs**
- **Login Dialog**: Email/password input with validation
- **2FA Dialog**: TOTP code input with timer
- **Password Prompt**: Third-party website credential storage
- **Error Handling**: User-friendly error messages

### **Browser Interface**
- **Address Bar**: URL input with search integration
- **Tab Bar**: Multi-tab management with thumbnails
- **Bookmarks Bar**: Quick access to saved sites
- **Status Bar**: Loading progress and security indicators

## 🔧 Configuration

### **Default Configuration**
```json
{
    "browser": {
        "name": "Zepra Core Browser",
        "version": "1.0.0",
        "userAgent": "Zepra Core/1.0",
        "defaultSearchEngine": "ketivee",
        "homepage": "https://ketivee.com"
    },
    "authentication": {
        "serverUrl": "https://auth.ketivee.com",
        "sessionTimeout": 604800,
        "autoRefresh": true,
        "secureMode": true
    },
    "ui": {
        "theme": "dark",
        "fontSize": 16,
        "showBookmarksBar": true
    },
    "security": {
        "enableSandbox": true,
        "blockAds": true,
        "blockTrackers": true
    }
}
```

### **Environment Variables**
- `ZEPRA_CONFIG_PATH`: Configuration directory
- `ZEPRA_ASSETS_PATH`: Assets directory
- `ZEPRA_DATA_PATH`: User data directory

## 🛠️ Development

### **Project Structure**
```
zepra/
├── src/
│   ├── main.cpp                 # Application entry point
│   ├── auth/
│   │   └── zepra_auth.cpp      # Authentication system
│   ├── ui/
│   │   ├── auth_ui.cpp         # Authentication UI
│   │   ├── window.cpp          # Window management
│   │   └── tab_manager.cpp     # Tab handling
│   ├── engine/
│   │   ├── html_parser.cpp     # HTML parsing
│   │   ├── webkit_engine.cpp   # WebKit integration
│   │   └── dev_tools.cpp       # Developer tools
│   ├── search/
│   │   └── ketivee_search.cpp  # Search integration
│   └── config/
│       └── config_manager.cpp   # Configuration management
├── include/
│   ├── auth/
│   │   └── zepra_auth.h        # Authentication headers
│   ├── ui/
│   │   ├── auth_ui.h           # UI headers
│   │   └── window.h
│   └── engine/
│       └── webkit_engine.h
├── assets/
│   ├── fonts/                  # Font files
│   ├── icons/                  # Browser icons
│   └── themes/                 # UI themes
├── config/
│   └── zepra_config.json       # Configuration file
└── build/
    └── bin/
        └── zepra               # Executable
```

### **Building for Development**
```bash
# Debug build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)

# With sanitizers
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined" ..
make -j$(nproc)
```

### **Testing**
```bash
# Run unit tests
make test

# Run integration tests
./test/integration_tests.sh

# Run authentication tests
./test/auth_tests.sh
```

## 📦 Installation

### **System Installation**
```bash
# Build the browser
./build-zepra-core.sh

# Install system-wide
sudo ./install-zepra.sh

# Run from anywhere
zepra
```

### **User Installation**
```bash
# Install for current user
./install-zepra.sh

# Run from anywhere
zepra
```

## 🔍 Search Integration

### **KetiveeSearch Features**
- **Fast Results**: Optimized search algorithms
- **Privacy**: No tracking or data collection
- **Customizable**: Easy to switch between search engines
- **Local Search**: Offline search capabilities
- **Smart Suggestions**: Context-aware search suggestions

## 🛡️ Security Features

### **Built-in Protection**
- **Ad Blocking**: Automatic ad and tracker blocking
- **HTTPS Enforcement**: Secure connections by default
- **Content Filtering**: Safe browsing with customizable filters
- **Sandbox Mode**: Isolated browsing for enhanced security
- **Privacy Controls**: Granular privacy settings

### **Authentication Security**
- **Token-based**: JWT tokens for session management
- **HTTP-Only Cookies**: Secure cookie storage
- **Domain Validation**: Whitelist-based access control
- **Session Timeout**: Configurable session limits
- **2FA Support**: Two-factor authentication

## 🎯 Performance

### **Optimizations**
- **Memory Management**: Smart tab and resource management
- **Hardware Acceleration**: OpenGL rendering
- **Caching**: Intelligent page and resource caching
- **Compression**: Gzip and Brotli support
- **Parallel Processing**: Multi-threaded rendering

### **Benchmarks**
- **Startup Time**: < 2 seconds
- **Memory Usage**: < 100MB base
- **Tab Memory**: ~50MB per tab
- **Rendering**: 60 FPS smooth scrolling

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

### **Development Guidelines**
- Follow C++17 standards
- Use consistent naming conventions
- Add unit tests for new features
- Update documentation
- Follow security best practices

## 📄 License

This project is licensed under the **Ketivee App Developer License v1.0** - see [LICENSE_DEV.txt](LICENSE_DEV.txt) for details.

## 🆘 Support

- **Website**: https://ketivee.org
- **Documentation**: [docs/](docs/)
- **Issues**: [GitHub Issues](https://github.com/ketivee/zepra-browser/issues)
- **Discussions**: [GitHub Discussions](https://github.com/ketivee/zepra-browser/discussions)
- **Email**: support@ketivee.com

## 🏆 Acknowledgments

- **Ketivee Team** - For the amazing platform and support
- **SDL2 Community** - For the excellent cross-platform library
- **OpenSSL Team** - For the robust cryptography library
- **CURL Team** - For the reliable HTTP client library
- **Open Source Community** - For inspiration and contributions

## 🚀 Roadmap

### **v1.1.0** (Next Release)
- [ ] Enhanced password manager
- [ ] Browser extensions support
- [ ] Sync across devices
- [ ] Advanced privacy controls
- [ ] Developer tools integration

### **v1.2.0** (Future)
- [ ] Mobile app companion
- [ ] Cloud sync
- [ ] Advanced security features
- [ ] Performance optimizations
- [ ] Accessibility improvements

---

**Built with ❤️ by the Ketivee Team**

*Zepra Core - Where speed meets elegance* 🦓⚡ 