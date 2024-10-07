link_libraries(${GMP_LIB})
add_definitions(${GMP_DEFINIONS})

OPTION(USE_CUDA "whether integrate with gpu" OFF) # Enabled by default

if(USE_CUDA)
    message("USE CUDA")

    add_definitions(-DUSE_CUDA)

    # if(${CMAKE_VERSION} VERSION_LESS "3.24.0")
    #     set(CMAKE_CUDA_ARCHITECTURES ${CUDA_ARCH})
    # else()
    #     set(CMAKE_CUDA_ARCHITECTURES native) # on 3.24+, on earlier it is ignored, and the target is not passed
    # endif()

    if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
        set(CMAKE_CUDA_ARCHITECTURES 70)
    endif()

    project(ffiasm LANGUAGES CXX CUDA ASM)
    find_package(CUDA REQUIRED)
    enable_language("CUDA")

    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        message("Build on Release Mode")
        set(CMAKE_CUDA_FLAGS_RELEASE "") # without this setting, the msm_g2 will panic with memory segmentation, should separate this fore G1 and G2
    else()
        message("Build on Debug Mode")
        set(CMAKE_CUDA_FLAGS_DEBUG "${CMAKE_CUDA_FLAGS_DEBUG} -g -G -O0")
    endif()

    # set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} --expt-relaxed-constexpr")

    # set(CUDA_SEPARABLE_COMPILATION ON)

    set(PROJECT_CUDA_SOURCE_DIR ${CMAKE_SOURCE_DIR}/depends/ffiasm/depends/cryptography_cuda/cuda)
    include_directories(SYSTEM ${CUDA_INCLUDE_DIRS})
    include_directories(${PROJECT_CUDA_SOURCE_DIR})
    include_directories("../depends/ffiasm/depends/cryptography_cuda/depends/blst/src") # the cuda library is dependent on blst
    set(LIB_SOURCES
        ../depends/ffiasm/depends/cryptography_cuda/src/lib.h
        ../depends/ffiasm/depends/cryptography_cuda/src/lib.cu
        ../depends/ffiasm/depends/cryptography_cuda/cuda/util/all_gpus.cpp
    )
    add_library(cryptography_cuda STATIC ${LIB_SOURCES})

    # # todo: pass feature as environment variable, or as option
    target_compile_definitions(cryptography_cuda PUBLIC -DFEATURE_BN254 -DG2_ENABLED -D__ADX__)
    target_link_libraries(cryptography_cuda ${CMAKE_SOURCE_DIR}/depends/ffiasm/depends/cryptography_cuda/depends/blst/libblst.a)
endif()

if(USE_ASM)
    if(ARCH MATCHES "arm64")
        add_definitions(-DUSE_ASM -DARCH_ARM64)
    elseif(ARCH MATCHES "x86_64")
        add_definitions(-DUSE_ASM -DARCH_X86_64)
    endif()
endif()

if(USE_ASM AND ARCH MATCHES "x86_64")
    if(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin")
        set(NASM_FLAGS -fmacho64 --prefix _)
    else()
        set(NASM_FLAGS -felf64 -DPIC)
    endif()

    add_custom_command(OUTPUT ${CMAKE_SOURCE_DIR}/build/fq_asm.o
        COMMAND nasm ${NASM_FLAGS} fq.asm -o fq_asm.o
        DEPENDS ${CMAKE_SOURCE_DIR}/build/fq.asm
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/build)

    add_custom_command(OUTPUT ${CMAKE_SOURCE_DIR}/build/fr_asm.o
        COMMAND nasm ${NASM_FLAGS} fr.asm -o fr_asm.o
        DEPENDS ${CMAKE_SOURCE_DIR}/build/fr.asm
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/build)
endif()

set(FR_SOURCES
    ../build/fr.hpp
    ../build/fr.cpp
)

if(USE_ASM)
    if(ARCH MATCHES "arm64")
        set(FR_SOURCES ${FR_SOURCES} ../build/fr_raw_arm64.s ../build/fr_raw_generic.cpp ../build/fr_generic.cpp)
    elseif(ARCH MATCHES "x86_64")
        set(FR_SOURCES ${FR_SOURCES} ../build/fr_asm.o)
    endif()
else()
    set(FR_SOURCES ${FR_SOURCES} ../build/fr_generic.cpp ../build/fr_raw_generic.cpp)
endif()

add_library(fr STATIC ${FR_SOURCES})
set_target_properties(fr PROPERTIES POSITION_INDEPENDENT_CODE ON)

link_libraries(fr)

set(FQ_SOURCES
    ../build/fq.hpp
    ../build/fq.cpp
)

if(USE_ASM)
    if(ARCH MATCHES "arm64")
        set(FQ_SOURCES ${FQ_SOURCES} ../build/fq_raw_arm64.s ../build/fq_raw_generic.cpp ../build/fq_generic.cpp)
    elseif(ARCH MATCHES "x86_64")
        set(FQ_SOURCES ${FQ_SOURCES} ../build/fq_asm.o)
    endif()
else()
    set(FQ_SOURCES ${FQ_SOURCES} ../build/fq_raw_generic.cpp ../build/fq_generic.cpp)
endif()

add_library(fq STATIC ${FQ_SOURCES})
set_target_properties(fq PROPERTIES POSITION_INDEPENDENT_CODE ON)

link_libraries(fq)

if(OpenMP_CXX_FOUND)
    add_definitions(-DUSE_OPENMP)
    add_compile_options(${OpenMP_CXX_FLAGS})
endif()

set(LIB_SOURCES
    binfile_utils.cpp
    zkey_utils.cpp
    wtns_utils.cpp
    logger.cpp
    fileloader.cpp
    groth16.hpp
    cuda.cpp
    prover.cpp
    fullprover.cpp
    ../depends/ffiasm/c/misc.cpp
    ../depends/ffiasm/c/naf.cpp
    ../depends/ffiasm/c/splitparstr.cpp
    ../depends/ffiasm/c/alt_bn128.cpp
)

if(USE_LOGGER)
    set(LIB_SOURCES ${LIB_SOURCES} logger.cpp)
    add_definitions(-DUSE_LOGGER)
endif()

add_library(rapidsnark SHARED ${LIB_SOURCES})
target_include_directories(
    rapidsnark
    PUBLIC
    ../src
    ../build
    ../depends/ffiasm/c
    ../depends/json/single_include
)

if(TARGET_PLATFORM MATCHES "macos_arm64")
    target_include_directories(rapidsnark PUBLIC ${GMP_INCLUDE_DIR})
    target_include_directories(rapidsnark PUBLIC ${LIB_OMP_PREFIX}/include)
    target_link_libraries(rapidsnark PUBLIC ${LIB_OMP_PREFIX}/lib/libomp.dylib)

    target_include_directories(fr PUBLIC ${GMP_INCLUDE_DIR})
    target_include_directories(fr PUBLIC ${LIB_OMP_PREFIX}/include)
    target_link_libraries(fr PUBLIC ${LIB_OMP_PREFIX}/lib/libomp.dylib)

    target_include_directories(fq PUBLIC ${GMP_INCLUDE_DIR})
    target_include_directories(fq PUBLIC ${LIB_OMP_PREFIX}/include)
    target_link_libraries(fq PUBLIC ${LIB_OMP_PREFIX}/lib/libomp.dylib)
endif()

# add_library(rapidsnarkStatic STATIC ${LIB_SOURCES})
# set_target_properties(rapidsnarkStatic PROPERTIES OUTPUT_NAME rapidsnark)

# add_library(rapidsnarkStaticFrFq STATIC ${LIB_SOURCES} ${FQ_SOURCES} ${FR_SOURCES})
# set_target_properties(rapidsnarkStaticFrFq PROPERTIES POSITION_INDEPENDENT_CODE ON)
# set_target_properties(rapidsnarkStaticFrFq PROPERTIES OUTPUT_NAME rapidsnark-fr-fq)

add_executable(
    prover
    main_prover.cpp
)

target_link_libraries(prover rapidsnark)
if(OpenMP_CXX_FOUND)
    if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
        target_link_libraries(prover OpenMP::OpenMP_CXX)
    endif()
endif()

if(USE_CUDA)
    target_link_libraries(prover cryptography_cuda)
endif(USE_CUDA)


if(USE_LOGGER OR NOT USE_OPENMP)
    target_link_libraries(prover pthread)
endif()

if(USE_SODIUM)
    target_link_libraries(prover sodium)
endif()

add_executable(test_prover test_prover.cpp)
target_include_directories(
    test_prover
    PUBLIC
    ../src
    ../build
    ../depends/ffiasm/c
    ../depends/json/single_include
)

if(BUILD_SERVER)
    add_executable(
        proverServer
        main_proofserver.cpp
        proverapi.cpp
    )

    target_include_directories(proverServer PRIVATE ../depends/pistache/include/)
    target_include_directories(proverServer PRIVATE ../depends/pistache/build/src)

    if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
        target_link_libraries(proverServer rapidsnark ${CMAKE_SOURCE_DIR}/depends/pistache/build/src/libpistache.dylib ${LIB_EVENT_DIR}/libevent.dylib ${LIB_EVENT_DIR}/libevent_pthreads.dylib)
    elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
        message("BUILD prover server for LINUX")
        target_link_directories(proverServer PRIVATE ${CMAKE_SOURCE_DIR}/depends/pistache/build/src)
        target_link_libraries(proverServer PRIVATE pistache rapidsnark OpenMP::OpenMP_CXX)
        if(USE_CUDA)
            target_link_libraries(proverServer PRIVATE cryptography_cuda)
        endif()
    endif()
endif()