# rapidsnark

Rapidsnark is a zkSnark proof generation written in C++ and intel/arm assembly. That generates proofs created in [circom](https://github.com/iden3/snarkjs) and [snarkjs](https://github.com/iden3/circom) very fast.

## Dependencies
### dev tools

You should have installed gcc, cmake, libsodium, and gmp (development)

- On ubuntu:

```
sudo apt-get install build-essential cmake libgmp-dev libsodium-dev nasm curl m4
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

## build
### macos_arm64
```
./build_gmp.sh macos_arm64
npx task buildPistacheMac # if server is needed
mkdir -p build_prover_macos_arm64 && cd build_prover_macos_arm64
cmake .. -DTARGET_PLATFORM=macos_arm64 -DBUILD_SERVER=ON -DLIB_EVENT_DIR=/opt/homebrew/opt/libevent/lib -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../package -DUSE_OPENMP=ON -DLIB_OMP_PREFIX=/opt/homebrew/opt/libomp/ -DGMP_INCLUDE_DIR=/opt/homebrew/include -DGMP_LIB_DIR=/opt/homebrew/lib -DUSE_LOGGER=ON
make -j4 && make install
```




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


export DYLD_LIBRARY_PATH=/usr/local/lib:depends/pistache/build/src:$DYLD_LIBRARY_PATH
