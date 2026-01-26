#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CIRCUIT_DIR="${SCRIPT_DIR}/circuits"
BUILD_DIR="${SCRIPT_DIR}/build"

# Check if circom is available
CIRCOM_BIN="circom"
if [ ! -f "$CIRCOM_BIN" ]; then
    CIRCOM_BIN=$(which circom)
fi

if [ -z "$CIRCOM_BIN" ] || [ ! -f "$CIRCOM_BIN" ]; then
    echo "Error: circom not found. Please install circom or place it in ./bin/"
    exit 1
fi

echo "Using circom: $CIRCOM_BIN"

# Create build directory
mkdir -p "${BUILD_DIR}"

# Compile 02x03 circuit
echo "Compiling 02x03 circuit..."
"$CIRCOM_BIN" \
    --r1cs \
    --sym \
    --wasm \
    --c \
    "${CIRCUIT_DIR}/02x03.circom" \
    --output "${BUILD_DIR}"

if [ $? -ne 0 ]; then
    echo "Error: circom compilation failed"
    exit 1
fi

echo "Circuit compiled successfully"

# Build C++ witness generator
echo "Building C++ witness generator..."
cd "${BUILD_DIR}/02x03_cpp"

# Patch Makefile to include nlohmann/json and gmp paths
sed -i '' 's|CFLAGS=-std=c++11 -O3 -I.|CFLAGS=-std=c++11 -O3 -I. -I../../depends/json/single_include -I/opt/homebrew/include|' Makefile
sed -i '' 's|-lgmp|-L/opt/homebrew/lib -lgmp|' Makefile

# For ARM64 (Apple Silicon), use generic C++ implementation instead of x86_64 assembly
if [ "$(uname -m)" = "arm64" ]; then
    echo "Detected ARM64, using generic C++ implementation..."
    # Copy generic implementations
    cp "${SCRIPT_DIR}/build/fr_generic.cpp" fr.cpp
    cp "${SCRIPT_DIR}/build/fr_raw_generic.cpp" fr_raw.cpp
    # Update Makefile: remove fr_asm.o, add fr_raw.o
    sed -i '' 's|fr_asm.o|fr_raw.o|' Makefile
    # Remove the nasm rule and add C++ rule for fr_raw
    sed -i '' '/^fr_asm.o:/,/$(NASM)/c\
fr_raw.o: fr_raw.cpp\
	$(CC) -c fr_raw.cpp $(CFLAGS)
' Makefile
fi

# Check if make is available
if ! command -v make &> /dev/null; then
    echo "Error: make not found"
    exit 1
fi

make

if [ $? -ne 0 ]; then
    echo "Error: C++ build failed"
    exit 1
fi

# Copy executable to build directory
cp 02x03 "${BUILD_DIR}/"

echo ""
echo "Build complete!"
echo "Witness generator: ${BUILD_DIR}/02x03"
echo "WASM: ${BUILD_DIR}/02x03_js/02x03.wasm"
echo "R1CS: ${BUILD_DIR}/02x03.r1cs"
