# rapidsnark

Rapidsnark is a zkSnark proof generation written in C++ and intel/arm assembly. That generates proofs created in [circom](https://github.com/iden3/snarkjs) and [snarkjs](https://github.com/iden3/circom) very fast.

## Dependencies
### zk tools
- install circom
```
git clone https://github.com/iden3/circom.git
cd circom
cargo build --release
cargo install --path circom
```

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
brew install nasm
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

## build circuits
```
curl -L -o powersOfTau28_hez_final_15.ptau https://storage.googleapis.com/zkevm/ptau/powersOfTau28_hez_final_15.ptau
./build_circuit.sh
npx snarkjs groth16 setup build/02x03.r1cs powersOfTau28_hez_final_15.ptau zkeys/02x03_new.zkey
npx snarkjs zkey export verificationkey zkeys/02x03_new.zkey zkeys/02x03.vkey.json
mv zkeys/02x03_new.zkey zkeys/02x03.zkey
```

## run rapidsnark server
```
<!-- export DYLD_LIBRARY_PATH=/usr/local/lib:$DYLD_LIBRARY_PATH -->
/usr/local/bin/proverServer 8080 zkeys/02x03.zkey
```