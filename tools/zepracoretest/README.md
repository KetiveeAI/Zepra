# Zepra Browser Python Development Tools

This directory contains a comprehensive Python-based development workflow system for the Zepra Browser project. The system provides validation, configuration management, analytics, build orchestration, and testing capabilities while keeping C++ for core performance-critical functionality.

## 🏗️ Architecture Overview

The system follows a hybrid architecture:
- **Python**: Used for validation, configuration, analytics, build orchestration, and testing
- **C++**: Used for core browser functionality and performance-critical operations
- **Shell/Batch**: Used for cross-platform workflow execution

## 📁 Directory Structure

```
tools/zepracoretest/
├── validation/
│   └── validation_engine.py      # Configuration and environment validation
├── config/
│   └── config_manager.py         # Browser and build configuration management
├── analytics/
│   └── analytics_engine.py       # Performance metrics and user behavior analytics
├── build/
│   └── build_manager.py          # C++ build orchestration and dependency management
├── test/
│   └── test_orchestrator.py      # Test execution and reporting
├── main_orchestrator.py          # Main workflow coordinator
├── requirements.txt              # Python dependencies
├── run_workflow.sh               # Linux/macOS workflow runner
├── run_workflow.bat              # Windows workflow runner
└── README.md                     # This file
```

## 🚀 Quick Start

### Prerequisites

- Python 3.8 or higher
- CMake 3.16 or higher
- C++ compiler (g++, clang++, or MSVC)
- SDL2, OpenGL, libcurl development libraries

### Setup

1. **Clone the repository** (if not already done):
   ```bash
   git clone <repository-url>
   cd zepra
   ```

2. **Navigate to the tools directory**:
   ```bash
   cd tools/zepracoretest
   ```

3. **Setup Python environment**:
   ```bash
   # Linux/macOS
   ./run_workflow.sh --setup
   
   # Windows
   run_workflow.bat --setup
   ```

4. **Run the full workflow**:
   ```bash
   # Linux/macOS
   ./run_workflow.sh
   
   # Windows
   run_workflow.bat
   ```

## 🔧 Components

### 1. Validation Engine (`validation/validation_engine.py`)

Validates the development environment and project configuration:

- **Environment Validation**: Checks Python version, CMake, C++ compiler
- **Configuration Validation**: Validates JSON configuration files
- **Code Structure Validation**: Ensures required directories and files exist

**Usage**:
```bash
python validation/validation_engine.py
```

### 2. Configuration Manager (`config/config_manager.py`)

Manages all browser and build configurations:

- **Browser Configuration**: Window settings, features, search engine URL
- **Build Configuration**: Compiler settings, build type, parallel jobs
- **Test Configuration**: Test timeouts, coverage settings, output formats

**Usage**:
```bash
python config/config_manager.py
```

### 3. Analytics Engine (`analytics/analytics_engine.py`)

Collects and analyzes browser performance and user behavior:

- **Performance Metrics**: CPU, memory, disk I/O, network usage
- **User Behavior**: Page loads, searches, tab switches, session data
- **Browser Statistics**: Tab counts, cache size, extension data

**Usage**:
```bash
python analytics/analytics_engine.py
```

### 4. Build Manager (`build/build_manager.py`)

Orchestrates the C++ build process:

- **Dependency Management**: Checks and manages build dependencies
- **CMake Integration**: Generates and manages CMake configurations
- **Multi-target Building**: Builds browser, tests, and tools
- **Parallel Builds**: Optimizes build performance

**Usage**:
```bash
python build/build_manager.py
```

### 5. Test Orchestrator (`test/test_orchestrator.py`)

Manages comprehensive testing:

- **Unit Tests**: Individual component testing
- **Integration Tests**: Component interaction testing
- **Performance Tests**: Benchmark and performance testing
- **Test Reporting**: JSON and HTML test reports

**Usage**:
```bash
python test/test_orchestrator.py
```

## 🎯 Workflow Options

The system supports multiple workflow options:

### Individual Workflows

```bash
# Validation only
./run_workflow.sh validation

# Configuration only
./run_workflow.sh config

# Build only
./run_workflow.sh build

# Tests only
./run_workflow.sh test

# Analytics only
./run_workflow.sh analytics
```

### Full Workflow

```bash
# Run complete workflow (default)
./run_workflow.sh
```

### Utility Commands

```bash
# Setup Python environment only
./run_workflow.sh --setup

# Clean build artifacts
./run_workflow.sh --clean

# Show help
./run_workflow.sh --help
```

## 📊 Output and Reports

The system generates various outputs:

### Generated Files

- `workflow_report.json` - Complete workflow execution report
- `test_output/` - Test results and reports
- `analytics_data/` - Performance and behavior analytics
- `configs/` - Configuration files
- `build/` - Build artifacts and CMake files

### Report Types

1. **Workflow Report**: Overall execution status and timing
2. **Test Reports**: JSON and HTML test results with coverage
3. **Analytics Reports**: Performance summaries and user behavior data
4. **Configuration Reports**: Environment variables and build settings

## 🔧 Configuration

### Browser Configuration

Edit `configs/browser_config.json` to customize browser settings:

```json
{
  "browser_name": "Zepra Browser",
  "version": "1.0.0",
  "window_width": 1200,
  "window_height": 800,
  "max_tabs": 50,
  "enable_developer_tools": true,
  "enable_analytics": true,
  "search_engine_url": "https://ketivee.com/search"
}
```

### Build Configuration

Edit `configs/build_config.json` to customize build settings:

```json
{
  "compiler": "g++",
  "build_type": "Release",
  "enable_tests": true,
  "enable_debug": false,
  "parallel_builds": 4
}
```

## 🐛 Troubleshooting

### Common Issues

1. **Python not found**:
   ```bash
   # Install Python 3.8+
   sudo apt install python3 python3-venv  # Ubuntu/Debian
   brew install python3                   # macOS
   ```

2. **Missing dependencies**:
   ```bash
   # Install system dependencies
   sudo apt install cmake build-essential libsdl2-dev libcurl4-openssl-dev
   ```

3. **Build failures**:
   ```bash
   # Clean and retry
   ./run_workflow.sh --clean
   ./run_workflow.sh build
   ```

4. **Test failures**:
   ```bash
   # Check test environment
   python test/test_orchestrator.py
   ```

### Debug Mode

Enable verbose output for debugging:

```bash
python main_orchestrator.py --verbose --workflow validation
```

## 🔄 Integration with C++ Core

The Python tools integrate seamlessly with the C++ core:

1. **Build Integration**: Python orchestrates C++ compilation via CMake
2. **Test Integration**: Python runs C++ test executables and parses results
3. **Configuration Integration**: Python generates C++ configuration headers
4. **Analytics Integration**: Python collects data from C++ browser runtime

### C++ Integration Points

- **Environment Variables**: Python sets environment variables for C++ compilation
- **CMake Generation**: Python generates CMakeLists.txt with proper settings
- **Test Execution**: Python runs C++ test binaries and processes output
- **Performance Monitoring**: Python collects metrics from running C++ processes

## 📈 Performance Benefits

This hybrid approach provides several benefits:

1. **Fast Development**: Python enables rapid tool development and iteration
2. **Performance**: C++ handles performance-critical browser operations
3. **Flexibility**: Python tools can be easily modified without recompilation
4. **Cross-platform**: Works on Windows, macOS, and Linux
5. **Extensibility**: Easy to add new tools and workflows

## 🤝 Contributing

To add new tools or modify existing ones:

1. **Add new Python modules** in appropriate subdirectories
2. **Update main_orchestrator.py** to include new workflows
3. **Update requirements.txt** for new dependencies
4. **Update run scripts** for new workflow options
5. **Add tests** for new functionality

### Code Style

- Follow PEP 8 for Python code
- Use type hints for function parameters
- Add docstrings for all classes and methods
- Include error handling and logging

## 📝 License

This project is part of the Zepra Browser project and follows the same licensing terms.

## 🆘 Support

For issues and questions:

1. Check the troubleshooting section above
2. Review the generated logs and reports
3. Run individual components for detailed error messages
4. Check the main project documentation

---

**Note**: This Python tooling system is designed to complement the C++ core browser functionality, providing a robust development environment while maintaining performance for the critical browser operations. 