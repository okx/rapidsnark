#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"

# Use LOCAL_CIRCUITS_PATH env var if set, otherwise fall back to local circuits dir
CIRCUIT_DIR="${LOCAL_CIRCUITS_PATH:-${SCRIPT_DIR}/circuits}"

# Check if circom is available (use CIRCOM_BIN_PATH env var if set)
CIRCOM_BIN="${CIRCOM_BIN_PATH:-$(which circom)}"

if [ -z "$CIRCOM_BIN" ] || [ ! -f "$CIRCOM_BIN" ]; then
    echo "Error: circom not found. Set CIRCOM_BIN_PATH or install circom to PATH"
    exit 1
fi

echo "Using circom: $CIRCOM_BIN"

# Create build directory
mkdir -p "${BUILD_DIR}"

# Compile 02x03 circuit
CIRCUIT_FILE="${CIRCUIT_DIR}/src/generated/02x03.circom"
if [ ! -f "$CIRCUIT_FILE" ]; then
    # Fall back to local circuits dir
    CIRCUIT_FILE="${SCRIPT_DIR}/circuits/02x03.circom"
fi

echo "Compiling circuit from: $CIRCUIT_FILE"
"$CIRCOM_BIN" \
    --r1cs \
    --sym \
    --wasm \
    --c \
    "$CIRCUIT_FILE" \
    --output "${BUILD_DIR}"

if [ $? -ne 0 ]; then
    echo "Error: circom compilation failed"
    exit 1
fi

echo "Circuit compiled successfully"

# Build C++ witness generator
echo "Building C++ witness generator..."
cd "${BUILD_DIR}/02x03_cpp"

# Patch Makefile for include and lib paths
if [[ "$(uname)" == "Darwin" ]]; then
    # macOS (both ARM64 and x86_64)
    sed -i '' 's|CFLAGS=-std=c++11 -O3 -I.|CFLAGS=-std=c++11 -O3 -I. -I../../depends/json/single_include -I/opt/homebrew/include -I/usr/local/include|' Makefile
    sed -i '' 's|-lgmp|-L/opt/homebrew/lib -L/usr/local/lib -lgmp|' Makefile
else
    # Linux
    sed -i 's|CFLAGS=-std=c++11 -O3 -I.|CFLAGS=-std=c++11 -O3 -I. -I../../depends/json/single_include|' Makefile
fi

make

if [ $? -ne 0 ]; then
    echo "Error: C++ build failed, falling back to WASM wrapper..."
    cd "$SCRIPT_DIR"
    cat > "${BUILD_DIR}/02x03" << 'WRAPPER'
#!/bin/bash
# WASM-based witness generator wrapper (fallback)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WASM_FILE="${SCRIPT_DIR}/02x03_js/02x03.wasm"
WITNESS_JS="${SCRIPT_DIR}/02x03_js/generate_witness.js"
INPUT_FILE="$1"
WITNESS_FILE="$2"
node "$WITNESS_JS" "$WASM_FILE" "$INPUT_FILE" "$WITNESS_FILE"
WRAPPER
    chmod +x "${BUILD_DIR}/02x03"
else
    cp 02x03 "${BUILD_DIR}/"
fi

echo ""
echo "Build complete!"
echo "Witness generator: ${BUILD_DIR}/02x03"
echo "WASM: ${BUILD_DIR}/02x03_js/02x03.wasm"
echo "R1CS: ${BUILD_DIR}/02x03.r1cs"
