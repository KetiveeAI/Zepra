#!/bin/bash

echo "🦓 Building Zepra Browser v1.0.0"
echo "================================="

# Check if CMake is available
if ! command -v cmake &> /dev/null; then
    echo "❌ CMake not found. Please install CMake."
    exit 1
fi

# Check if a C++ compiler is available
if ! command -v g++ &> /dev/null && ! command -v clang++ &> /dev/null; then
    echo "❌ No C++ compiler found. Please install g++ or clang++."
    exit 1
fi

# Detect OS and set appropriate settings
if [[ "$OSTYPE" == "darwin"* ]]; then
    echo "✅ Detected macOS"
    GENERATOR="Unix Makefiles"
    # Check for Homebrew SDL2
    if brew list sdl2 &> /dev/null; then
        echo "✅ Found SDL2 via Homebrew"
    else
        echo "⚠️  SDL2 not found via Homebrew. Installing..."
        brew install sdl2
    fi
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    echo "✅ Detected Linux"
    GENERATOR="Unix Makefiles"
    # Check for SDL2 development packages
    if ! pkg-config --exists sdl2; then
        echo "⚠️  SDL2 development packages not found."
        echo "   On Ubuntu/Debian: sudo apt-get install libsdl2-dev"
        echo "   On Fedora: sudo dnf install SDL2-devel"
        echo "   On Arch: sudo pacman -S sdl2"
        exit 1
    fi
else
    echo "⚠️  Unknown OS type: $OSTYPE"
    GENERATOR="Unix Makefiles"
fi

# Create build directory
mkdir -p build
cd build

# Configure with CMake
echo ""
echo "🔧 Configuring project..."
cmake -G "$GENERATOR" -DCMAKE_BUILD_TYPE=Debug ..
if [ $? -ne 0 ]; then
    echo "❌ CMake configuration failed"
    exit 1
fi

# Build the project
echo ""
echo "🔨 Building project..."
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
if [ $? -ne 0 ]; then
    echo "❌ Build failed"
    exit 1
fi

echo ""
echo "✅ Build completed successfully!"
echo ""
echo "🚀 To run Zepra Browser:"
echo "   cd build"
echo "   ./zepra"
echo ""

# Create UEFI disk image
dd if=/dev/zero of=neolyx.img bs=1G count=20
parted neolyx.img mklabel gpt
parted neolyx.img mkpart EFI fat32 1MiB 512MiB
parted neolyx.img mkpart NXFS 512MiB 100%

# Format partitions
mkfs.fat -F32 /dev/sda1
./nxfs-format /dev/sda2