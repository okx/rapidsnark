###

#Build targets
host:
	rm -rf build_prover_linux_x86_64 && mkdir build_prover_linux_x86_64 && cd build_prover_linux_x86_64 && \
	cmake .. -DBUILD_SERVER=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../package -DUSE_OPENMP=ON -DUSE_LOGGER=ON && \
	make -j$(nproc) -vvv && make install

host-gpu:
	rm -rf build_prover_linux_x86_64_with_gpu && mkdir build_prover_linux_x86_64_with_gpu && cd build_prover_linux_x86_64_with_gpu && \
	cmake .. -DBUILD_SERVER=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../package -DUSE_OPENMP=ON -DUSE_LOGGER=ON -DUSE_CUDA=ON && \
	make -j$(nproc) -vvv && make install

macos_arm64:
	rm -rf build_prover_macos_arm64 && mkdir -p build_prover_macos_arm64 && cd build_prover_macos_arm64 && \
	cmake .. -DTARGET_PLATFORM=macos_arm64 -DBUILD_SERVER=ON -DLIB_EVENT_DIR=/opt/homebrew/opt/libevent/lib -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../package -DUSE_OPENMP=ON -DLIB_OMP_PREFIX=/opt/homebrew/opt/libomp/ -DGMP_INCLUDE_DIR=/opt/homebrew/include -DGMP_LIB_DIR=/opt/homebrew/lib -DUSE_LOGGER=ON && \
	make -j$(nproc) -vvv && make install

android:
	rm -rf build_prover_android && mkdir build_prover_android && cd build_prover_android && \
	cmake .. -DTARGET_PLATFORM=ANDROID -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../package_android && \
	make -j$(nproc) -vvv && make install

android_x86_64:
	rm -rf build_prover_android_x86_64 && mkdir build_prover_android_x86_64 && cd build_prover_android_x86_64 && \
	cmake .. -DTARGET_PLATFORM=ANDROID_x86_64 -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../package_android_x86_64 && \
	make -j$(nproc) -vvv && make install

ios:
	rm -rf build_prover_ios && mkdir build_prover_ios && cd build_prover_ios && \
	cmake .. -GXcode -DTARGET_PLATFORM=IOS -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../package_ios && \
    echo "" && echo "Now open Xcode and compile the generated project" && echo ""

clean:
	rm -rf build_prover build_prover_android build_prover_android_x86_64 build_prover_ios package package_android \
		package_android_x86_64 package_ios depends/gmp/package depends/gmp/package_android_arm64 \
		depends/gmp/package_android_x86_64 depends/gmp/package_ios_arm64
