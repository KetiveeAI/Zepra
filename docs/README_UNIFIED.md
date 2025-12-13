# Zepra Core Browser - Unified System

🦓 **Zepra Core Browser** is a modern, secure web browser built with C++ and SDL2, featuring integrated authentication, download management, developer tools, and sandbox security.

## 🚀 Features

### Core System
- **Unified Architecture**: Single integrated system combining UI, engine, and all subsystems
- **Cross-Platform**: Windows, Linux, and macOS support
- **Modern C++**: Built with C++17 and modern development practices
- **SDL2 Graphics**: Hardware-accelerated rendering with OpenGL support

### Authentication & Security
- **Ketivee Integration**: Seamless authentication with Ketivee services
- **2FA Support**: Two-factor authentication with TOTP codes
- **Session Management**: Secure cookie-based session handling
- **Sandbox Security**: Isolated execution environment for web content

### Download Management
- **Integrated Download Manager**: Built-in download system with progress tracking
- **Resume Support**: Pause and resume downloads
- **Speed Monitoring**: Real-time download speed display
- **File Organization**: Automatic file organization and naming

### Developer Tools
- **Integrated Console**: Built-in JavaScript console for debugging
- **Network Inspector**: Monitor network requests and responses
- **Element Inspector**: Inspect and modify DOM elements
- **Performance Profiler**: Analyze page performance

### Search Integration
- **Ketivee Search**: Integrated search with Ketivee's search engine
- **Smart Suggestions**: Context-aware search suggestions
- **Search History**: Persistent search history and bookmarks

## 🛠️ Building

### Prerequisites

#### Linux (Ubuntu/Debian)
```bash
sudo apt update
sudo apt install build-essential cmake libsdl2-dev libsdl2-ttf-dev libsdl2-image-dev
sudo apt install libcurl4-openssl-dev libssl-dev libjson-c-dev
sudo apt install libopengl0 libgl1-mesa-dev
```

#### Windows
- Install Visual Studio 2019 or later with C++ support
- Install CMake 3.15 or later
- Install vcpkg and install dependencies:
```cmd
vcpkg install sdl2 sdl2-ttf sdl2-image curl openssl json-c
```

#### macOS
```bash
brew install cmake sdl2 sdl2_ttf sdl2_image curl openssl json-c
```

### Building the Unified System

#### Linux/macOS
```bash
# Clone the repository
git clone https://github.com/ketivee/zepra-core-browser.git
cd zepra-core-browser

# Make build script executable
chmod +x build_unified.sh

# Build the unified system
./build_unified.sh
```

#### Windows
```cmd
# Clone the repository
git clone https://github.com/ketivee/zepra-core-browser.git
cd zepra-core-browser

# Build the unified system
build_unified.bat
```

### Manual Build
```bash
mkdir build_unified
cd build_unified
cmake .. -DCMAKE_BUILD_TYPE=Release -DUSE_UNIFIED_SYSTEM=ON
make -j$(nproc)
```

## 🚀 Running

### Starting the Browser
```bash
cd build_unified
./zepra_core_browser
```

### Command Line Options
```bash
./zepra_core_browser [options]
  --config <file>     Load configuration from file
  --auth-server <url> Set authentication server URL
  --download-dir <dir> Set download directory
  --debug             Enable debug mode
  --help              Show help information
```

## 🎮 Controls

### General Navigation
- **F12**: Toggle Developer Tools
- **F5**: Refresh current page
- **Ctrl+R**: Refresh current page
- **Ctrl+Shift+R**: Hard refresh (clear cache)
- **Ctrl+T**: New tab
- **Ctrl+W**: Close current tab
- **Ctrl+Tab**: Next tab
- **Ctrl+Shift+Tab**: Previous tab

### Download Manager
- **Ctrl+Shift+D**: Show/Hide Download Manager
- **Ctrl+J**: Open Downloads folder
- **Space**: Pause/Resume selected download
- **Delete**: Cancel selected download

### Developer Tools
- **F12**: Toggle Developer Tools
- **Ctrl+Shift+I**: Open Inspector
- **Ctrl+Shift+J**: Open Console
- **Ctrl+Shift+C**: Element Inspector mode

### Authentication
- **Ctrl+Shift+L**: Logout
- **Ctrl+Shift+A**: Show Account Settings

## 🔧 Configuration

### Configuration File
Create `config/zepra_config.json`:
```json
{
  "auth": {
    "serverUrl": "https://auth.ketivee.com",
    "timeout": 30000,
    "enable2FA": true
  },
  "downloads": {
    "defaultDirectory": "./downloads",
    "maxConcurrent": 3,
    "enableResume": true
  },
  "security": {
    "enableSandbox": true,
    "blockPopups": true,
    "enableContentFilter": true
  },
  "ui": {
    "theme": "dark",
    "fontSize": 14,
    "enableAnimations": true
  },
  "developer": {
    "enableDevTools": true,
    "enableConsole": true,
    "enableInspector": true
  }
}
```

### Environment Variables
```bash
export ZEPRA_AUTH_SERVER=https://auth.ketivee.com
export ZEPRA_DOWNLOAD_DIR=./downloads
export ZEPRA_DEBUG=true
```

## 🔐 Authentication

### Login Process
1. **Email/Password**: Enter your Ketivee account credentials
2. **2FA Verification**: If enabled, enter your TOTP code
3. **Session Management**: Browser maintains secure session cookies

### Security Features
- **HTTP-Only Cookies**: Secure cookie storage
- **Token Refresh**: Automatic token renewal
- **Session Validation**: Regular session verification
- **Secure Storage**: Encrypted credential storage

## 📥 Download Manager

### Features
- **Progress Tracking**: Real-time download progress
- **Speed Monitoring**: Download speed display
- **Resume Support**: Pause and resume downloads
- **File Organization**: Automatic file naming and organization
- **Error Handling**: Comprehensive error reporting

### Usage
1. **Start Download**: Right-click link → "Save As" or use download manager
2. **Monitor Progress**: View progress in download manager
3. **Control Downloads**: Pause, resume, or cancel downloads
4. **Access Files**: Open completed downloads directly

## 🛠️ Developer Tools

### Console
- **JavaScript Console**: Execute JavaScript commands
- **Error Logging**: View JavaScript errors and warnings
- **Network Logging**: Monitor network requests
- **Performance Metrics**: View timing information

### Inspector
- **DOM Inspection**: Examine page structure
- **Element Selection**: Click to inspect elements
- **Style Editing**: Modify CSS styles in real-time
- **Layout Analysis**: View element dimensions and positioning

### Network Panel
- **Request Monitoring**: View all network requests
- **Response Analysis**: Examine response headers and content
- **Timing Information**: View request timing details
- **Error Tracking**: Monitor failed requests

## 🔍 Search Integration

### Ketivee Search
- **Smart Suggestions**: Context-aware search suggestions
- **Search History**: Persistent search history
- **Bookmarks**: Save and organize bookmarks
- **Search Filters**: Advanced search filtering options

### Search Features
- **Auto-complete**: Intelligent search suggestions
- **Search History**: Browse previous searches
- **Bookmark Management**: Organize and categorize bookmarks
- **Search Filters**: Filter results by type, date, etc.

## 🛡️ Security

### Sandbox Environment
- **Isolated Execution**: Web content runs in isolated environment
- **Resource Limits**: Memory and CPU usage limits
- **Network Restrictions**: Controlled network access
- **File System Protection**: Restricted file system access

### Content Security
- **XSS Protection**: Cross-site scripting protection
- **CSRF Protection**: Cross-site request forgery prevention
- **Content Filtering**: Malicious content detection
- **Popup Blocking**: Unwanted popup prevention

## 🐛 Troubleshooting

### Common Issues

#### Build Errors
```bash
# SDL2 not found
sudo apt install libsdl2-dev

# CURL not found
sudo apt install libcurl4-openssl-dev

# OpenSSL not found
sudo apt install libssl-dev
```

#### Runtime Errors
```bash
# Font not found
# Copy fonts to assets/fonts/ directory

# Configuration file not found
# Create config/zepra_config.json

# Permission denied
# Check file permissions for downloads directory
```

#### Authentication Issues
- Verify server URL in configuration
- Check network connectivity
- Ensure 2FA codes are current
- Clear browser cache and cookies

### Debug Mode
```bash
./zepra_core_browser --debug
```

### Log Files
Logs are written to:
- Linux: `~/.zepra/logs/`
- Windows: `%APPDATA%\Zepra\logs\`
- macOS: `~/Library/Logs/Zepra/`

## 🤝 Contributing

### Development Setup
1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests for new features
5. Submit a pull request

### Code Style
- Follow C++17 standards
- Use meaningful variable names
- Add comments for complex logic
- Include error handling
- Write unit tests

### Testing
```bash
# Run unit tests
make test

# Run integration tests
make integration-test

# Run performance tests
make performance-test
```

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- **SDL2**: Cross-platform multimedia library
- **CURL**: Network transfer library
- **OpenSSL**: Cryptography library
- **json-c**: JSON parsing library
- **Ketivee**: Authentication and search services

## 📞 Support

- **Documentation**: [docs.ketivee.com](https://docs.ketivee.com)
- **Issues**: [GitHub Issues](https://github.com/ketivee/zepra-core-browser/issues)
- **Discussions**: [GitHub Discussions](https://github.com/ketivee/zepra-core-browser/discussions)
- **Email**: support@ketivee.com

---

**🦓 Zepra Core Browser** - Modern, Secure, Integrated Web Browsing 