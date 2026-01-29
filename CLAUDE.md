# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

Rapidsnark is a high-performance zkSNARK proof generator written in C++ and assembly (Intel x86_64 and ARM64). It generates Groth16 proofs for circuits created with circom and snarkjs. This is a new implementation that supersedes the old rapidsnark repository.

## Build System Architecture

### Multi-Platform CMake Build

The build system uses CMake with platform-specific configurations defined in `cmake/platform.cmake`. The main platforms are:

- **Linux x86_64**: Standard host builds with optional GPU support (CUDA)
- **Linux ARM64 (aarch64)**: Server deployments on ARM hardware
- **macOS ARM64**: Apple Silicon (M1/M2/M3)
- **macOS x86_64**: Intel Macs
- **Android**: Both ARM64 and x86_64
- **iOS**: Both device and simulator

### Field Arithmetic Code Generation

A critical part of the build is generating optimized field arithmetic code for the BN128 curve:
- Field element types **Fq** and **Fr** are generated at build time by `depends/ffiasm` (a JavaScript-based field arithmetic code generator)
- Generates C++ code (`fq.cpp`, `fr.cpp`, `fq.hpp`, `fr.hpp`) and assembly (`fq.asm`, `fr.asm` for x86_64, or `.s` files for ARM64)
- Assembly versions use NASM for x86_64 and native ARM64 assembly for ARM platforms
- Generic C++ fallbacks are available when `USE_ASM=NO`
- All generated files land in the `build/` directory

The field generation process:
1. Run `node depends/ffiasm/src/buildzqfield.js` with modulus parameters
2. For Fq: modulus = 21888242871839275222246405745257275088696311157297823662689037894645226208583
3. For Fr: modulus = 21888242871839275222246405745257275088548364400416034343698204186575808495617
4. Assemble `.asm` files with NASM (platform-specific flags)

### Dependencies

**System libraries:**
- GMP (GNU Multiple Precision): Built from source via `build_gmp.sh` for most targets, or use system version (macOS ARM64)
- OpenMP: For parallel computation (optional but recommended)
- libsodium: For randomness
- NASM: x86_64 assembly
- libevent: Required for prover server mode

**Vendored (git submodules in `depends/`):**
- `json` (nlohmann/json): JSON parsing
- `ffiasm`: Field arithmetic code generator
- `circom_runtime`: Circom witness calculator runtime
- `pistache`: HTTP server library (for server mode only)

## Building

### Initial Setup

```bash
# Initialize submodules
git submodule update --init --recursive

# Build GMP for your platform
./build_gmp.sh host              # Linux x86_64
./build_gmp.sh macos_arm64       # macOS Apple Silicon
./build_gmp.sh android           # Android ARM64
```

### Build Targets (Makefile)

The root Makefile provides convenience targets:

```bash
# Standalone prover builds
make host                   # Linux x86_64 with server mode
make host-gpu              # Linux x86_64 with CUDA GPU support
make macos_arm64           # macOS Apple Silicon
make host_arm64            # Linux ARM64 cross-compile
make android               # Android ARM64
make ios                   # iOS device

# Prover server (auto-detects platform)
make prover-server         # Builds server binary for current platform

# Clean all builds
make clean
```

Build output goes to `package/bin/`:
- `prover`: Standalone CLI proof generator
- `proverServer`: HTTP API server (if `BUILD_SERVER=ON`)
- `verifier`: Proof verifier
- `test_prover`: Test harness

### Direct CMake Builds

For more control, use CMake directly:

```bash
mkdir build_custom && cd build_custom
cmake .. \
  -DTARGET_PLATFORM=macos_arm64 \
  -DBUILD_SERVER=ON \
  -DCMAKE_BUILD_TYPE=Release \
  -DUSE_OPENMP=ON \
  -DUSE_ASM=ON \
  -DUSE_LOGGER=ON \
  -DGMP_INCLUDE_DIR=/opt/homebrew/include \
  -DGMP_LIB_DIR=/opt/homebrew/lib

make -j$(nproc)
make install  # Installs to CMAKE_INSTALL_PREFIX
```

**Important CMake flags:**
- `BUILD_SERVER`: Enable HTTP server mode (requires pistache)
- `USE_ASM`: Enable assembly optimizations (default ON)
- `USE_OPENMP`: Enable parallel computation (default ON)
- `USE_LOGGER`: Enable logging support
- `USE_CUDA`: Enable GPU acceleration (Linux only)
- `TARGET_PLATFORM`: Cross-compilation target (see `cmake/platform.cmake`)

## Circuit Development Workflow

### Building Test Circuits

```bash
# Download powers of tau ceremony file (needed for setup)
curl -L -o powersOfTau28_hez_final_15.ptau \
  https://storage.googleapis.com/zkevm/ptau/powersOfTau28_hez_final_15.ptau

# Install Node.js dependencies (circomlib, snarkjs)
npm install

# Compile circuit (generates .r1cs, .wasm, C++ witness generator)
./build_circuit.sh

# Setup zkey (Groth16 proving key)
npx snarkjs groth16 setup build/02x03.r1cs powersOfTau28_hez_final_15.ptau zkeys/02x03_new.zkey

# Export verification key
npx snarkjs zkey export verificationkey zkeys/02x03_new.zkey zkeys/02x03.vkey.json

# Rename to final zkey
mv zkeys/02x03_new.zkey zkeys/02x03.zkey
```

**Circuit paths:** `build_circuit.sh` looks for circuits in:
1. `$LOCAL_CIRCUITS_PATH` (environment variable override)
2. `circuits/src/generated/02x03.circom`
3. Fallback: `circuits/02x03.circom`

**Circom binary:** Set `$CIRCOM_BIN_PATH` if circom is not in `$PATH`.

### Building Witness Generator with ARM64 Assembly

For ARM64 systems (Linux aarch64), use `build_circuit_arm64.sh` instead of `build_circuit.sh`. This script automatically detects ARM64 architecture and:
- Copies ARM64 assembly files (`fr_raw_arm64.s`, `fq_raw_arm64.s`) to the witness generator directory
- Creates an ARM64-optimized Makefile using the system assembler (`as`)
- Links against ARM64 assembly for 3-5x performance improvement

```bash
# Generate field arithmetic with ARM64 assembly
cd build
node ../depends/ffiasm/src/buildzqfield.js -q 21888242871839275222246405745257275088548364400416034343698204186575808495617 -n Fr
node ../depends/ffiasm/src/buildzqfield.js -q 21888242871839275222246405745257275088696311157297823662689037894645226208583 -n Fq
cd ..

# Build circuit with ARM64 assembly support
./build_circuit_arm64.sh

# Verify ARM64 symbols
nm build/02x03 | grep Fr_raw
```

See `README_WITGEN_ARM64.md` for detailed instructions on building and testing in Docker.

### Generating Witnesses

After building a circuit, you can generate witnesses using the C++ witness calculator:

```bash
# Using C++ witness generator (faster)
./build/02x03 input.json witness.wtns

# Or using WASM (if C++ build failed)
node build/02x03_js/generate_witness.js \
  build/02x03_js/02x03.wasm \
  input.json \
  witness.wtns
```

## Running the Prover

### CLI Mode

```bash
# Generate proof from witness file
./package/bin/prover <zkey_file> <witness_file> <proof.json> <public.json>
```

### Server Mode

The prover server exposes an HTTP API for generating proofs:

```bash
# Start server with one or more circuits
./package/bin/proverServer <port> <circuit1.zkey> [circuit2.zkey ...]

# Example
/usr/local/bin/proverServer 8080 zkeys/02x03.zkey
```

**API Endpoints:**
- `GET /status`: Get server status and loaded circuits
- `POST /start`: Start the prover
- `POST /stop`: Stop the prover
- `POST /input/:circuit`: Submit witness input for proof generation (circuit name is zkey filename without extension)
- `POST /cancel`: Cancel current proof generation

The server uses Pistache (HTTP library) and supports concurrent requests with a single-threaded event loop. The `FullProver` class manages proof generation state and supports multiple pre-loaded circuits.

## Code Architecture

### Core Components

**`src/prover.h` / `prover.cpp`**: C API for proof generation
- `groth16_prover_create()`: Initialize prover from zkey
- `groth16_prover_prove()`: Generate proof from witness
- `groth16_prover_destroy()`: Cleanup
- Error codes: `PROVER_OK`, `PROVER_ERROR`, `PROVER_ERROR_SHORT_BUFFER`, `PROVER_INVALID_WITNESS_LENGTH`

**`src/groth16.hpp` / `groth16.cpp`**: Groth16 implementation
- Template-based cryptographic engine (`Engine` = `AltBn128::Engine`)
- `Prover<Engine>`: Core proving algorithm using FFT and multi-scalar multiplication (MSM)
- `Verifier<Engine>`: Pairing-based verification
- `Proof<Engine>`: Proof data structure (A, B, C points)

**`src/fullprover.hpp` / `fullprover.cpp`**: Multi-circuit stateful prover
- Manages multiple pre-loaded circuits (map of circuit name -> prover)
- Thread-safe proof generation with status tracking
- Used by the server API to avoid re-loading zkeys per request

**`src/alt_bn128.hpp` (from `depends/ffiasm/c/`)**: BN128 curve implementation
- Elliptic curve point arithmetic (G1, G2)
- Field extensions (Fq, Fq2, Fq6, Fq12)
- Pairing operations

**`src/zkey_utils.cpp` / `wtns_utils.cpp`**: Binary file parsers
- Parse `.zkey` files (proving keys in iden3 binary format)
- Parse `.wtns` files (witness data)

**`src/logger.cpp`**: Logging infrastructure
- Singleton logger with thread-safe file/console output
- Log levels: DEBUG, INFO, WARNING, ERROR
- Controlled by `USE_LOGGER` CMake flag

### Key Data Structures

**Witness (wtns)**: Array of field elements representing circuit variable assignments
- Format: Binary file with section-based structure
- Sections: Header (1), Witness data (2)

**Zkey**: Proving key for Groth16
- Contains: Elliptic curve points for A, B1, B2, C, H; verification key elements; R1CS coefficients
- Parsed into memory-mapped structures for efficient access
- Pre-computation of FFT roots for polynomial operations

**Proof**: Three elliptic curve points (JSON serialization)
```json
{
  "pi_a": ["<G1 x>", "<G1 y>", "1"],
  "pi_b": [[G2 components]],
  "pi_c": ["<G1 x>", "<G1 y>", "1"]
}
```

### Build Artifacts

After a full build, key files:
- `build/fq.cpp`, `build/fr.cpp`: Generated field arithmetic
- `build/fq.asm`, `build/fr.asm`: x86_64 assembly (or `*_arm64.s` for ARM)
- `build/<circuit>_cpp/`: C++ witness generator
- `build/<circuit>.r1cs`: Constraint system
- `zkeys/<circuit>.zkey`: Proving key

## Development Notes

### Performance Considerations

- **Assembly optimizations** (`USE_ASM=ON`): 3-5x speedup for field operations. Essential for production.
- **OpenMP** (`USE_OPENMP=ON`): Parallelizes MSM and FFT. Near-linear scaling up to ~16 cores.
- **GPU acceleration** (`USE_CUDA=ON`, Linux only): Offloads MSM to CUDA. Requires NVIDIA GPU.
- **Memory**: Large circuits (>1M constraints) may need 8GB+ RAM for proof generation.

### Platform-Specific Notes

**macOS ARM64:**
- Use Homebrew-installed dependencies: `brew install gmp libomp nasm libevent`
- GMP is dynamically linked (libgmp.dylib) from system paths
- Must specify `LIB_OMP_PREFIX=/opt/homebrew/opt/libomp` for OpenMP

**Linux:**
- Static linking preferred for GMP to avoid runtime dependencies
- CUDA support only available on Linux x86_64
- Server mode uses dynamically linked pistache

**Android:**
- Requires `ANDROID_NDK` environment variable pointing to NDK root
- ELF page alignment forced to 16k for Android 14+ compatibility
- OpenMP available but optional (use `android_openmp` target)

### Testing

```bash
# Run built-in tests
cd testdata
../package/bin/test_public_size circuit_final.zkey 86

# Full end-to-end test
./build/02x03 build/input_02x03.json build/02x03.wtns
./package/bin/prover zkeys/02x03.zkey build/02x03.wtns proof.json public.json
```

### Modifying Field Arithmetic

If you need to support a different curve:
1. Modify `tasksfile.js` `createFieldSources()` function with new modulus
2. Run `npm run task createFieldSources` to regenerate field code
3. Update `alt_bn128.cpp` elliptic curve parameters

### Server Deployment

For production server deployments:
- Build with `-DCMAKE_BUILD_TYPE=Release` for optimizations
- Enable logging with `-DUSE_LOGGER=ON` for debugging
- The server is single-threaded (Pistache with 1 thread) but can handle concurrent requests via async I/O
- Consider running behind a reverse proxy (nginx) for TLS and rate limiting
- Pre-load all circuits at startup (pass multiple `.zkey` files as arguments)

## Docker

### Basic ARM64 Prover Server

ARM64 Docker build:
```bash
docker build --platform linux/arm64 -t rapidsnark-arm64 -f Dockerfile.arm64 .
```

The Dockerfile builds pistache, compiles the prover server, and sets up the runtime environment.

### ARM64 with Witness Generator and Assembly Optimizations

For a complete build including circom witness generator with ARM64 assembly:

```bash
# Using Docker Compose (recommended - mounts circuits directory)
docker-compose -f docker-compose.arm64.yml up --build

# Or build directly
docker build --platform linux/arm64 -t rapidsnark-arm64-witgen -f Dockerfile.arm64.witgen .

# Run with circuits mounted
docker run --rm -it --platform linux/arm64 \
  -v /path/to/circuits:/workspace/circuits:ro \
  rapidsnark-arm64-witgen bash
```

This extended Dockerfile:
- Installs Rust and builds circom from source
- Generates field arithmetic code with ARM64 assembly (`fr_raw_arm64.s`, `fq_raw_arm64.s`)
- Builds both the prover server and witness generator
- Includes verification steps to confirm ARM64 assembly symbols are present

Inside the container, test the witness generator:
```bash
# Generate witness with ARM64 assembly optimizations
./build/02x03 circuits/input.json outputs/witness.wtns

# Generate proof
./package/bin/prover circuit.zkey outputs/witness.wtns proof.json public.json
```

See `README_WITGEN_ARM64.md` for comprehensive Docker testing guide.
