#!/bin/bash

echo "🦓 Building Zepra Core Browser - Unified System"
echo "================================================"

# Create build directory
mkdir -p build_unified
cd build_unified

# Configure with CMake
echo "📋 Configuring build..."
cmake .. -DCMAKE_BUILD_TYPE=Release -DUSE_UNIFIED_SYSTEM=ON

# Build the project
echo "🔨 Building Zepra Core Browser..."
make -j$(nproc)

# Check if build was successful
if [ $? -eq 0 ]; then
    echo "✅ Build successful!"
    echo "🚀 Executable: build_unified/zepra_core_browser"
    echo ""
    echo "To run the browser:"
    echo "  cd build_unified"
    echo "  ./zepra_core_browser"
    echo ""
    echo "Features included:"
    echo "  ✅ Unified UI and Engine"
    echo "  ✅ Integrated Download Manager"
    echo "  ✅ Developer Tools Console"
    echo "  ✅ Authentication System"
    echo "  ✅ Ketivee Search Integration"
    echo "  ✅ Sandbox Security"
    echo ""
else
    echo "❌ Build failed!"
    exit 1
fi 