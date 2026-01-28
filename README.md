## Important note 

**This is a new implementation of rapidsnark. The original (and now obsoleted) implementation is available here: [rapidsnark-old](https://github.com/iden3/rapidsnark-old).**

# rapidsnark

Rapidsnark is a fast zkSnark proof generator written in C++ and Intel/ARM assembly. It generates proofs for circuits created with [circom](https://github.com/iden3/circom) and [snarkjs](https://github.com/iden3/snarkjs).

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

## build in standalone mode
### x86_64
```
git submodule update --init --recursive
./build_gmp.sh host
make host
```


## build with server mode
```
./build_pistache.sh
make prover-server
```

### if want to test build in docker
```
docker build --platform linux/arm64 -t rapidsnark-arm64 -f Dockerfile.arm64 .
```

## build circuits
```
curl -L -o powersOfTau28_hez_final_15.ptau https://storage.googleapis.com/zkevm/ptau/powersOfTau28_hez_final_15.ptau
npm install
./build_circuit.sh
npx snarkjs groth16 setup build/02x03.r1cs powersOfTau28_hez_final_15.ptau zkeys/02x03_new.zkey
npx snarkjs zkey export verificationkey zkeys/02x03_new.zkey zkeys/02x03.vkey.json
mv zkeys/02x03_new.zkey zkeys/02x03.zkey
```

## run rapidsnark server
```
/usr/local/bin/proverServer 8080 zkeys/02x03.zkey
```