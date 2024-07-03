## Important note

**This is a new implementation of rapidsnark. The original (and now obsoleted) implemenation is available here: [rapidsnark-old](https://github.com/iden3/rapidsnark-old).**

# rapidsnark

Rapidsnark is a zkSnark proof generation written in C++ and intel/arm assembly. That generates proofs created in [circom](https://github.com/iden3/snarkjs) and [snarkjs](https://github.com/iden3/circom) very fast.

## Dependencies

You should have installed gcc, cmake, libsodium, and gmp (development)

In ubuntu:

```
sudo apt-get install build-essential cmake libgmp-dev libsodium-dev nasm curl m4
```

## Compile prover in server mode

```sh
npm install
git submodule init
git submodule update
npm run task buildPistache
npm run task createFieldSources
npm run task buildProverServer
# Single Thread, for testing purposes
npm run task buildProverServerSingleThread
```

## Launch prover in server mode
```sh
export LD_LIBRARY_PATH=depends/pistache/build/src
./build_nodejs/proverServer  <port> <circuit1_zkey> <circuit2_zkey> ... <circuitN_zkey>
```

For every `circuit.circom` you have to generate with circom with --c option the `circuit_cpp` and after compilation you have to copy the executable into the `build` folder so the server can generate the witness and then the proof based on this witness.
You have an example of the usage calling the server endpoints to generate the proof with Nodejs in `/tools/request.js`.

To test a request you should pass an `input.json` as a parameter to the request call.
```sh
node tools/request.js <input.json> <circuit>
```


## Compile prover in standalone mode

### using cmake
the `CMakeLists.txt` int the root folder includes `cmake/platform.cmake` which setup the environment for `GMP`

#### Compile prover for x86_64 host machine

```sh
git submodule update --init
./build_gmp.sh host
make host
```

#### Compile standalone + GPU

```sh
git submodule update --init --recursive
./build_gmp.sh host
# build libblst.a manually
cd ./depends/ffiasm/depends/cryptography_cuda/depends/blst/
./build.sh
# cd back to rapidsnark
make host-gpu
```

#### Compile prover for macOS arm64 host machine

```sh
git submodule init
git submodule update
# ./build_gmp.sh macos_arm64 # this will produce some output files not whitlisted as per company policy
brew install gmp # install dependencies
brew install libomp
mkdir -p build_prover_macos_arm64 && cd build_prover_macos_arm64
cmake .. -DTARGET_PLATFORM=macos_arm64 -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../package -DUSE_LOGGER=ON  -DGMP_INCLUDE_DIR=/opt/homebrew/include -DGMP_LIB_DIR=/opt/homebrew/lib
make -j4 && make install
```

if want to also build proverServer
```sh
npx task buildPistacheMac
cmake .. -DTARGET_PLATFORM=macos_arm64 -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../package -DUSE_LOGGER=ON  -DGMP_INCLUDE_DIR=/opt/homebrew/include -DGMP_LIB_DIR=/opt/homebrew/lib -DBUILD_SERVER=ON
make -j4
```

#### Compile prover for linux arm64 host machine

```sh
git submodule init
git submodule update
./build_gmp.sh host
mkdir build_prover && cd build_prover
cmake .. -DTARGET_PLATFORM=arm64_host -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../package
make -j4 && make install
```

#### Compile prover for linux arm64 machine

```sh
git submodule init
git submodule update
./build_gmp.sh host
mkdir build_prover && cd build_prover
cmake .. -DTARGET_PLATFORM=aarch64 -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../package_aarch64
make -j4 && make install
```

#### Compile prover for Android

Install Android NDK from https://developer.android.com/ndk or with help of "SDK Manager" in Android Studio.

Set the value of ANDROID_NDK environment variable to the absolute path of Android NDK root directory.

Examples:

```sh
export ANDROID_NDK=/home/test/Android/Sdk/ndk/23.1.7779620  # NDK is installed by "SDK Manager" in Android Studio.
export ANDROID_NDK=/home/test/android-ndk-r23b              # NDK is installed as a stand-alone package.
```

Prerequisites if build on Ubuntu:

```sh
apt-get install curl xz-utils build-essential cmake m4 nasm
```

Compilation:

```sh
git submodule init
git submodule update
./build_gmp.sh android
mkdir build_prover_android && cd build_prover_android
cmake .. -DTARGET_PLATFORM=ANDROID -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../package_android
make -j4 && make install
```

#### Compile prover for iOS

Install Xcode

```sh
git submodule init
git submodule update
./build_gmp.sh ios
mkdir build_prover_ios && cd build_prover_ios
cmake .. -GXcode -DTARGET_PLATFORM=IOS -DCMAKE_INSTALL_PREFIX=../package_ios
xcodebuild -destination 'generic/platform=iOS' -scheme rapidsnarkStatic -project rapidsnark.xcodeproj -configuration Release
```
Open generated Xcode project and compile prover.

## Build for iOS emulator

Install Xcode

```sh
git submodule init
git submodule update
./build_gmp.sh ios_simulator
mkdir build_prover_ios_simulator && cd build_prover_ios_simulator
cmake .. -GXcode -DTARGET_PLATFORM=IOS -DCMAKE_INSTALL_PREFIX=../package_ios_simulator -DUSE_ASM=NO
xcodebuild -destination 'generic/platform=iOS Simulator' -scheme rapidsnarkStatic -project rapidsnark.xcodeproj
```

Files that you need to copy to your XCode project to link against Rapidsnark:
* build_prover_ios_simulator/src/Debug-iphonesimulator/librapidsnark.a
* build_prover_ios_simulator/src/Debug-iphonesimulator/libfq.a
* build_prover_ios_simulator/src/Debug-iphonesimulator/libfr.a
* depends/gmp/package_iphone_simulator/lib/libgmp.a

## Building proof

You have a full prover compiled in the build directory.

So you can replace snarkjs command:

```sh
snarkjs groth16 prove <circuit.zkey> <witness.wtns> <proof.json> <public.json>
```

by this one
```sh
./package/bin/prover <circuit.zkey> <witness.wtns> <proof.json> <public.json>
```


### using npm script
```
npm run task createFieldSources
npm run task buildProver
```
## Benchmark

This prover parallelizes as much as it can the proof generation.

The prover is much faster that snarkjs and faster than bellman.

[TODO] Some comparative tests should be done.


## License

rapidsnark is part of the iden3 project copyright 2021 0KIMS association and published with GPL-3 license. Please check the COPYING file for more details.



## dependencies
- ffiasm: master
- cryptography_cuda: dev
