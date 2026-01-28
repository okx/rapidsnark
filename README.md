## Important note 

**This is a new implementation of rapidsnark. The original (and now obsoleted) implementation is available here: [rapidsnark-old](https://github.com/iden3/rapidsnark-old).**

# rapidsnark

Rapidsnark is a fast zkSnark proof generator written in C++ and Intel/ARM assembly. It generates proofs for circuits created with [circom](https://github.com/iden3/circom) and [snarkjs](https://github.com/iden3/snarkjs).

## Dependencies
### dev tools

Install gcc, cmake, libsodium, and gmp (development libraries).

- On ubuntu:

```
sudo apt-get install build-essential cmake libgmp-dev libsodium-dev nasm curl m4
```

On MacOS:

```
brew install cmake gmp libsodium nasm
```
- On mac
```
brew install gmp # install dependencies
brew install libomp
brew install libevent # for server
```

### third party project deps
- nlohmann/json
- circom_runtime
- ffiasm
- pistache (for proof server)
```
git submodule update --init
```

## build with server mode
### macos_arm64
```
sudo ./build_pistache.sh

mkdir -p build_prover_macos_arm64 && cd build_prover_macos_arm64
cmake .. -DTARGET_PLATFORM=macos_arm64 \
           -DBUILD_SERVER=ON \
           -DLIB_EVENT_DIR=/opt/homebrew/opt/libevent/lib \
           -DCMAKE_BUILD_TYPE=Release \
           -DCMAKE_INSTALL_PREFIX=/usr/local \
           -DUSE_OPENMP=ON \
           -DLIB_OMP_PREFIX=/opt/homebrew/opt/libomp/ \
           -DGMP_INCLUDE_DIR=/opt/homebrew/include \
           -DGMP_LIB_DIR=/opt/homebrew/lib \
           -DUSE_LOGGER=ON
make -j4 && sudo make install
```

## run rapidsnark server
```
export DYLD_LIBRARY_PATH=/usr/local/lib:$DYLD_LIBRARY_PATH
/usr/local/bin/proverServer 8080 ~/data/xlayer/privacy/02x03.zkey
```