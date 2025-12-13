@echo off
echo 🦓 Building Zepra Browser v1.0.0
echo =================================

REM Check if CMake is available
cmake --version >nul 2>&1
if errorlevel 1 (
    echo ❌ CMake not found. Please install CMake and add it to your PATH.
    pause
    exit /b 1
)

REM Check if Visual Studio or MinGW is available
where cl >nul 2>&1
if errorlevel 1 (
    where g++ >nul 2>&1
    if errorlevel 1 (
        echo ❌ No C++ compiler found. Please install Visual Studio or MinGW.
        pause
        exit /b 1
    ) else (
        echo ✅ Found MinGW compiler
        set GENERATOR="MinGW Makefiles"
    )
) else (
    echo ✅ Found Visual Studio compiler
    set GENERATOR="Visual Studio 17 2022"
)

REM Create build directory
if not exist "build" mkdir build
cd build

REM Configure with CMake
echo.
echo 🔧 Configuring project...
cmake -G %GENERATOR% -DCMAKE_BUILD_TYPE=Debug ..
if errorlevel 1 (
    echo ❌ CMake configuration failed
    pause
    exit /b 1
)

REM Build the project
echo.
echo 🔨 Building project...
cmake --build . --config Debug
if errorlevel 1 (
    echo ❌ Build failed
    pause
    exit /b 1
)

echo.
echo ✅ Build completed successfully!
echo.
echo 🚀 To run Zepra Browser:
echo    cd build
echo    Debug\zepra.exe
echo.
pause 