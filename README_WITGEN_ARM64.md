# Building Witness Generator with ARM64 Assembly

This guide explains how to build the circom witness generator with ARM64 assembly optimizations in Docker.

## Background

The circom witness generator (C++ version) normally uses x86_64 NASM assembly for field arithmetic operations. On ARM64 systems, we need to use ARM64 assembly files (`fr_raw_arm64.s`, `fq_raw_arm64.s`) instead.

## Quick Start

### Option 1: Using Docker Compose (Recommended)

Build and test with your circuits mounted:

```bash
docker-compose -f docker-compose.arm64.yml up --build
```

This will:
1. Build the Docker image with all dependencies
2. Mount your circuits from `/Users/cliffyang/dev/okx/circuits-v2`
3. Generate field arithmetic code with ARM64 assembly
4. Build the witness generator with ARM64 optimizations
5. Drop you into an interactive shell for testing

### Option 2: Using the Build Script

```bash
./test_witgen_docker.sh
```

Then run the container interactively:

```bash
docker run --rm -it --platform linux/arm64 \
  -v /Users/cliffyang/dev/okx/circuits-v2:/workspace/circuits:ro \
  rapidsnark-arm64-witgen bash
```

### Option 3: Manual Docker Build

```bash
docker build --platform linux/arm64 \
  -t rapidsnark-arm64-witgen \
  -f Dockerfile.arm64.witgen .
```

## Architecture Details

### Field Arithmetic Code Generation

The build process generates optimized field arithmetic code:

```
build/
├── fr.hpp, fr.cpp           # Field Fr implementation
├── fq.hpp, fq.cpp           # Field Fq implementation
├── fr_raw_arm64.s           # ARM64 assembly for Fr (used on ARM)
├── fq_raw_arm64.s           # ARM64 assembly for Fq (used on ARM)
├── fr.asm                   # x86_64 assembly for Fr (used on Intel)
└── fq.asm                   # x86_64 assembly for Fq (used on Intel)
```

### Witness Generator Build Process

The `build_circuit_arm64.sh` script:

1. **Detects architecture** using `uname -m` (aarch64/arm64 vs x86_64)
2. **Compiles circuit** with circom to generate C++ code
3. **For ARM64:**
   - Copies `fr_raw_arm64.s` from parent build directory
   - Creates custom Makefile that uses system `as` assembler
   - Links against ARM64 assembly object files
4. **For x86_64:**
   - Uses circom-generated Makefile with NASM
   - Links against x86_64 assembly

### Generated Makefile (ARM64)

```makefile
CC=g++
CFLAGS=-std=c++11 -O3 -I. -I../../depends/json/single_include
AS=as
DEPS_HPP = circom.hpp calcwit.hpp fr.hpp
DEPS_O = main.o calcwit.o fr.o fr_raw_arm64.o

all: 02x03

%.o: %.cpp $(DEPS_HPP)
	$(CC) -c $< $(CFLAGS)

fr_raw_arm64.o: fr_raw_arm64.s
	$(AS) fr_raw_arm64.s -o fr_raw_arm64.o

02x03: $(DEPS_O) 02x03.o
	$(CC) -o 02x03 *.o -lgmp
```

## Verification

### Check ARM64 Assembly Symbols

Inside the container:

```bash
# Verify binary architecture
file /workspace/build/02x03
# Output: /workspace/build/02x03: ELF 64-bit LSB executable, ARM aarch64...

# Check for ARM64 assembly symbols
nm /workspace/build/02x03 | grep Fr_raw
```

Expected symbols:
- `Fr_rawAdd`
- `Fr_rawSub`
- `Fr_rawMMul` (Montgomery multiplication)
- `Fr_rawFromMontgomery`
- And many more...

### Performance Test

Compare ARM64 assembly vs generic C++:

```bash
# Time witness generation with assembly (default)
time /workspace/build/02x03 input.json witness_asm.wtns

# To build without assembly (for comparison):
# Edit Makefile and remove fr_raw_arm64.o, use fr_raw_generic.cpp instead
```

ARM64 assembly should be 3-5x faster than generic C++ for large circuits.

## Testing with Your Circuits

### Generate Witness

```bash
# Inside container with circuits mounted
cd /workspace

# Generate witness
./build/02x03 circuits/test/input.json outputs/witness.wtns

# Check output
ls -lh outputs/witness.wtns
```

### Generate Proof

```bash
# First, ensure you have a zkey file
# (Generate this outside container with: npx snarkjs groth16 setup ...)

# Generate proof with rapidsnark
./package/bin/prover \
  /path/to/circuit.zkey \
  outputs/witness.wtns \
  outputs/proof.json \
  outputs/public.json
```

## Troubleshooting

### "circom not found"

The Dockerfile installs circom from source. If it fails:
- Check Rust installation: `rustc --version`
- Manually build circom: `cd /tmp && git clone https://github.com/iden3/circom && cd circom && cargo build --release`

### "fr_raw_arm64.s: No such file"

The ARM64 assembly files must be generated first:
```bash
cd build
node ../depends/ffiasm/src/buildzqfield.js -q 21888242871839275222246405745257275088548364400416034343698204186575808495617 -n Fr
```

### "undefined reference to Fr_rawAdd"

The assembly file wasn't properly linked. Check:
```bash
ls -l build/02x03_cpp/fr_raw_arm64.o
nm build/02x03_cpp/fr_raw_arm64.o | grep Fr_raw
```

### Performance Not Improved

Verify assembly is actually being used:
```bash
# Should show fr_raw_arm64.o
ldd build/02x03

# Should show ARM assembly symbols
nm build/02x03 | grep Fr_raw | wc -l
# Should be > 20 symbols
```

## Environment Variables

- `LOCAL_CIRCUITS_PATH`: Path to circuits directory (default: `./circuits`)
- `CIRCOM_BIN_PATH`: Path to circom binary (default: `which circom`)

## Next Steps

- Benchmark witness generation performance vs WASM
- Test with production circuits at scale
- Profile memory usage for large circuits
- Consider GPU acceleration for even faster witness generation
