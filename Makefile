###

#Build targets
host:
	rm -rf build_prover && mkdir build_prover && cd build_prover && \
	cmake .. -DUSE_CUDA=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../package -DUSE_LOGGER=ON -DG2_ENABLED=ON && \
	make -j$(nproc) -vvv && make install

host-gpu:
	rm -rf build_prover_gpu && mkdir build_prover_gpu && cd build_prover_gpu && \
	cmake .. -DUSE_CUDA=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../package -DUSE_LOGGER=ON -DG2_ENABLED=ON && \
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
