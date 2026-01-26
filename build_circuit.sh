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

# For ARM64 (Apple Silicon), skip C++ build and use WASM wrapper
if [ "$(uname -m)" = "arm64" ]; then
    echo "Detected ARM64, creating WASM-based witness generator wrapper..."
    cat > "${BUILD_DIR}/02x03" << 'WRAPPER'
#!/bin/bash
# WASM-based witness generator wrapper for ARM64 compatibility
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WASM_FILE="${SCRIPT_DIR}/02x03_js/02x03.wasm"
INPUT_FILE="$1"
WITNESS_FILE="$2"
npx snarkjs wtns calculate "$WASM_FILE" "$INPUT_FILE" "$WITNESS_FILE"
WRAPPER
    chmod +x "${BUILD_DIR}/02x03"
else
    # Build C++ witness generator for x86_64
    echo "Building C++ witness generator..."
    cd "${BUILD_DIR}/02x03_cpp"

    # Patch Makefile to include nlohmann/json and gmp paths
    sed -i '' 's|CFLAGS=-std=c++11 -O3 -I.|CFLAGS=-std=c++11 -O3 -I. -I../../depends/json/single_include -I/opt/homebrew/include|' Makefile
    sed -i '' 's|-lgmp|-L/opt/homebrew/lib -lgmp|' Makefile

    make

    if [ $? -ne 0 ]; then
        echo "Error: C++ build failed"
        exit 1
    fi

    cp 02x03 "${BUILD_DIR}/"
fi

echo ""
echo "Build complete!"
echo "Witness generator: ${BUILD_DIR}/02x03"
echo "WASM: ${BUILD_DIR}/02x03_js/02x03.wasm"
echo "R1CS: ${BUILD_DIR}/02x03.r1cs"
