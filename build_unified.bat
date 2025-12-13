@echo off
echo 🦓 Building Zepra Core Browser - Unified System
echo ================================================

REM Create build directory
if not exist build_unified mkdir build_unified
cd build_unified

REM Configure with CMake
echo 📋 Configuring build...
cmake .. -DCMAKE_BUILD_TYPE=Release -DUSE_UNIFIED_SYSTEM=ON

REM Build the project
echo 🔨 Building Zepra Core Browser...
cmake --build . --config Release

REM Check if build was successful
if %ERRORLEVEL% EQU 0 (
    echo ✅ Build successful!
    echo 🚀 Executable: build_unified\zepra_core_browser.exe
    echo.
    echo To run the browser:
    echo   cd build_unified
    echo   zepra_core_browser.exe
    echo.
    echo Features included:
    echo   ✅ Unified UI and Engine
    echo   ✅ Integrated Download Manager
    echo   ✅ Developer Tools Console
    echo   ✅ Authentication System
    echo   ✅ Ketivee Search Integration
    echo   ✅ Sandbox Security
    echo.
) else (
    echo ❌ Build failed!
    exit /b 1
) 