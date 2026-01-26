#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PISTACHE_DIR="${SCRIPT_DIR}/depends/pistache"
BUILD_DIR="${PISTACHE_DIR}/build"

mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

cmake -G "Unix Makefiles" \
    -DCMAKE_BUILD_TYPE=Release \
    -DPISTACHE_BUILD_TESTS=OFF \
    -DLIB_EVENT_INCLUDE_DIR=/opt/homebrew/opt/libevent/include \
    ..

make -j$(sysctl -n hw.ncpu)

# Install library to /usr/local/lib
sudo cp -P "${BUILD_DIR}/src/libpistache"*.dylib /usr/local/lib/
sudo cp -P "${BUILD_DIR}/src/libpistache.a" /usr/local/lib/

# Install headers to /usr/local/include
sudo mkdir -p /usr/local/include/pistache
sudo cp -R "${PISTACHE_DIR}/include/pistache/"* /usr/local/include/pistache/
