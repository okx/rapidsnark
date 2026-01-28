#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PISTACHE_DIR="${SCRIPT_DIR}/depends/pistache"
BUILD_DIR="${PISTACHE_DIR}/build"

rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

CMAKE_EXTRA_FLAGS=""
if [[ "$(uname)" == "Darwin" ]]; then
    CMAKE_EXTRA_FLAGS="-DCMAKE_CXX_FLAGS=-I/opt/homebrew/opt/libevent/include -DCMAKE_EXE_LINKER_FLAGS=-L/opt/homebrew/opt/libevent/lib"
fi

cmake -G "Unix Makefiles" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
    -DPISTACHE_BUILD_TESTS=OFF \
    ${CMAKE_EXTRA_FLAGS} \
    ..

make -j$(sysctl -n hw.ncpu)

echo "Pistache built successfully at ${BUILD_DIR}/src/"
echo "Static library: ${BUILD_DIR}/src/libpistache.a"
