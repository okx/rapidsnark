# Multi-Stage Docker Build for Rapidsnark

## Overview

The Docker build is now split into **2 stages** for faster rebuilds:

```
┌─────────────────────────────────────┐
│  Stage 1: Dockerfile.arm64          │
│  Base Image (rapidsnark-arm64-base) │
│                                     │
│  ✓ Ubuntu 22.04 ARM64               │
│  ✓ System dependencies (GMP, etc)  │
│  ✓ Pistache HTTP server             │
│  ✓ Prover server binary             │
│                                     │
│  Build time: ~5-10 minutes          │
│  Size: ~500MB                       │
└─────────────────────────────────────┘
              ↓ extends
┌─────────────────────────────────────┐
│  Stage 2: Dockerfile.arm64.witgen   │
│  Extended (rapidsnark-arm64-witgen) │
│                                     │
│  ✓ Node.js & npm                    │
│  ✓ Rust & Cargo                     │
│  ✓ Circom compiler                  │
│  ✓ Field arithmetic (ARM64 asm)     │
│  ✓ Circuits (copied in)             │
│  ✓ Witness generator binary         │
│                                     │
│  Build time: ~3-5 minutes           │
│  Total size: ~2GB                   │
└─────────────────────────────────────┘
```

## Benefits

### ✅ Much Faster Rebuilds

**Before (monolithic):** 8-15 minutes every time
- Install all dependencies
- Build pistache
- Build prover server
- Install Rust/circom
- Build witness generator

**After (multi-stage):**
- **First build:** 8-15 minutes (same as before)
- **Subsequent builds:** 3-5 minutes (only stage 2)
  - Base image is cached!
  - Only rebuilds witness generator layer

### ✅ Flexibility

- Use base image alone for just prover server
- Extend base image for witness generator
- Easy to add more extensions (e.g., GPU support, different circuits)

### ✅ Better Caching

- Base dependencies rarely change → cached
- Circuits change often → only rebuilds stage 2
- Docker layer cache works optimally

## Build Methods

### Method 1: Smart Build Script (Recommended)

```bash
./build_docker_witgen.sh
```

This script:
1. Checks if base image exists
2. Asks if you want to rebuild base (usually "no")
3. Builds only witness generator layer (fast!)

**Output:**
```
✓ Base image 'rapidsnark-arm64-base' already exists
Rebuild base image? (y/N): n
Skipping base image build (using existing)

=== Step 2/2: Building witness generator image ===
This adds: Rust, circom, circuits, witness generator
This step takes ~3-5 minutes...
```

### Method 2: Docker Compose

```bash
# Build both stages
docker-compose -f docker-compose.arm64.yml build

# Or build and run
docker-compose -f docker-compose.arm64.yml up --build
```

### Method 3: Manual (Step by Step)

```bash
# Step 1: Build base image (once)
docker build --platform linux/arm64 \
  -t rapidsnark-arm64-base \
  -f Dockerfile.arm64 .

# Step 2: Build witness generator (extends base)
docker build --platform linux/arm64 \
  --build-arg BASE_IMAGE=rapidsnark-arm64-base \
  --build-arg CIRCUITS_PATH=../circuits-v2 \
  -t rapidsnark-arm64-witgen \
  -f Dockerfile.arm64.witgen .
```

## When to Rebuild Each Stage

### Rebuild Base Image When:
- ❌ System dependencies change (GMP, libsodium, etc.)
- ❌ Prover C++ code changes (`src/*.cpp`)
- ❌ CMakeLists.txt changes
- ❌ Makefile changes
- ✅ Once every few weeks/months (rarely)

### Rebuild Witness Generator When:
- ✅ Circuits change (very common)
- ✅ Circom version update
- ✅ Field arithmetic needs regeneration
- ✅ Testing different circuits
- ✅ Multiple times per day during development

## Typical Workflow

### Initial Setup (First Time)
```bash
# Build everything from scratch
./build_docker_witgen.sh
# Takes ~8-15 minutes
```

### Daily Development
```bash
# Only rebuild witness generator (circuits changed)
docker build --platform linux/arm64 \
  --build-arg BASE_IMAGE=rapidsnark-arm64-base \
  --build-arg CIRCUITS_PATH=../circuits-v2 \
  -t rapidsnark-arm64-witgen \
  -f Dockerfile.arm64.witgen .
# Takes ~3-5 minutes
```

### After Prover Code Changes
```bash
# Rebuild base image
docker build --platform linux/arm64 \
  -t rapidsnark-arm64-base \
  -f Dockerfile.arm64 .

# Then rebuild witness generator
docker build --platform linux/arm64 \
  --build-arg BASE_IMAGE=rapidsnark-arm64-base \
  -t rapidsnark-arm64-witgen \
  -f Dockerfile.arm64.witgen .
# Takes ~8-15 minutes total
```

## Image Sizes

Check your image sizes:
```bash
docker images | grep rapidsnark
```

**Expected:**
- `rapidsnark-arm64-base`: ~500MB
- `rapidsnark-arm64-witgen`: ~2GB (includes Rust, circom, circuits)

## Using the Images

### Base Image (Prover Server Only)
```bash
docker run --rm -it --platform linux/arm64 \
  -v ./zkeys:/workspace/zkeys:ro \
  rapidsnark-arm64-base \
  /workspace/package/bin/proverServer 8080 /workspace/zkeys/circuit.zkey
```

**Use when:**
- You only need proof generation
- Witnesses are generated elsewhere
- Minimizing image size

### Extended Image (With Witness Generator)
```bash
docker run --rm -it --platform linux/arm64 \
  -v ./docker-outputs:/workspace/outputs \
  rapidsnark-arm64-witgen bash
```

**Use when:**
- You need end-to-end: witness → proof
- Testing circuits
- Development environment

## Advanced: Multiple Witness Generator Images

Build different versions for different circuits:

```bash
# Build base once
docker build --platform linux/arm64 -t rapidsnark-arm64-base -f Dockerfile.arm64 .

# Build multiple witness generator images
docker build --platform linux/arm64 \
  --build-arg BASE_IMAGE=rapidsnark-arm64-base \
  --build-arg CIRCUITS_PATH=../circuits-v2 \
  -t rapidsnark-witgen-v2 \
  -f Dockerfile.arm64.witgen .

docker build --platform linux/arm64 \
  --build-arg BASE_IMAGE=rapidsnark-arm64-base \
  --build-arg CIRCUITS_PATH=../circuits-v3 \
  -t rapidsnark-witgen-v3 \
  -f Dockerfile.arm64.witgen .

# All share the same base layer (efficient!)
```

## Clean Up

### Remove witness generator only (keep base)
```bash
docker rmi rapidsnark-arm64-witgen
```

### Remove everything
```bash
docker rmi rapidsnark-arm64-witgen rapidsnark-arm64-base
```

### Remove dangling layers
```bash
docker image prune -f
```

## Troubleshooting

### Error: "BASE_IMAGE not found"

**Problem:** Trying to build witness generator before base image

**Solution:**
```bash
# Build base first
docker build --platform linux/arm64 -t rapidsnark-arm64-base -f Dockerfile.arm64 .
# Then build witness generator
./build_docker_witgen.sh
```

### Base image exists but changes aren't applied

**Problem:** Base image cached, need to rebuild

**Solution:**
```bash
# Force rebuild base
docker build --no-cache --platform linux/arm64 -t rapidsnark-arm64-base -f Dockerfile.arm64 .
```

### Want to force rebuild everything

```bash
# Remove all images and caches
docker rmi rapidsnark-arm64-witgen rapidsnark-arm64-base
docker builder prune -af

# Rebuild
./build_docker_witgen.sh
```

## Summary

**Key Points:**
- 🚀 **First build:** ~10 minutes
- ⚡ **Rebuilds:** ~3-5 minutes (only witness generator layer)
- 💾 **Base image:** Cached and reused
- 🔄 **Circuits changes:** Fast rebuilds
- 🎯 **Prover changes:** Rebuild base, then extend

**Build Script:**
```bash
./build_docker_witgen.sh  # Smart build with prompts
```

**Docker Compose:**
```bash
docker-compose -f docker-compose.arm64.yml up --build
```

**Manual:**
```bash
# Once: build base
docker build -t rapidsnark-arm64-base -f Dockerfile.arm64 .

# Often: rebuild witness generator
docker build --build-arg BASE_IMAGE=rapidsnark-arm64-base -t rapidsnark-arm64-witgen -f Dockerfile.arm64.witgen .
```
