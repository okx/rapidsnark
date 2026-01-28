link_libraries(${GMP_LIB})
add_definitions(${GMP_DEFINIONS})

if(USE_ASM)
    if(ARCH MATCHES "arm64")
        add_definitions(-DUSE_ASM -DARCH_ARM64)
    elseif(ARCH MATCHES "x86_64")
        add_definitions(-DUSE_ASM -DARCH_X86_64)
    endif()
endif()

if(DEFINED BITS_PER_CHUNK)
    add_definitions(-DMSM_BITS_PER_CHUNK=${BITS_PER_CHUNK})
endif()

if(USE_ASM AND ARCH MATCHES "x86_64")
    if (CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin" AND NOT TARGET_PLATFORM MATCHES "^android(_x86_64)?")
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
        set(FR_SOURCES ${FR_SOURCES} ../build/fr_raw_arm64.s ../build/fr_generic.cpp)
    elseif(ARCH MATCHES "x86_64")
        set(FR_SOURCES ${FR_SOURCES} ../build/fr_asm.o)
    endif()
else()
    set(FR_SOURCES ${FR_SOURCES} ../build/fr_generic.cpp ../build/fr_raw_generic.cpp)
endif()

add_library(fr STATIC ${FR_SOURCES})
set_target_properties(fr PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(fr PUBLIC ${CMAKE_SOURCE_DIR}/${GMP_INCLUDE_DIR})

link_libraries(fr)

set(FQ_SOURCES
    ../build/fq.hpp
    ../build/fq.cpp
)

if(USE_ASM)
    if(ARCH MATCHES "arm64")
        set(FQ_SOURCES ${FQ_SOURCES} ../build/fq_raw_arm64.s ../build/fq_generic.cpp)
    elseif(ARCH MATCHES "x86_64")
        set(FQ_SOURCES ${FQ_SOURCES} ../build/fq_asm.o)
    endif()
else()
    set(FQ_SOURCES ${FQ_SOURCES} ../build/fq_raw_generic.cpp ../build/fq_generic.cpp)
endif()

add_library(fq STATIC ${FQ_SOURCES})
set_target_properties(fq PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(fq PUBLIC ${CMAKE_SOURCE_DIR}/${GMP_INCLUDE_DIR})

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
    prover.cpp
    fullprover.cpp
    prover.h
    verifier.cpp
    verifier.h
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

add_library(rapidsnarkStatic STATIC ${LIB_SOURCES})
set_target_properties(rapidsnarkStatic PROPERTIES OUTPUT_NAME rapidsnark)
target_include_directories(
    rapidsnarkStatic
    PUBLIC
    ../src
    ../build
    ../depends/ffiasm/c
    ../depends/json/single_include
)

add_library(rapidsnarkStaticFrFq STATIC ${LIB_SOURCES} ${FQ_SOURCES} ${FR_SOURCES})
set_target_properties(rapidsnarkStaticFrFq PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(rapidsnarkStaticFrFq PROPERTIES OUTPUT_NAME rapidsnark-fr-fq)
target_include_directories(
    rapidsnarkStaticFrFq
    PUBLIC
    ../src
    ../build
    ../depends/ffiasm/c
    ../depends/json/single_include
    ${CMAKE_SOURCE_DIR}/${GMP_INCLUDE_DIR}
)

add_executable(
    prover
    main_prover.cpp
)

target_link_libraries(prover rapidsnarkStatic)
if(OpenMP_CXX_FOUND)
    if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
        target_link_libraries(prover OpenMP::OpenMP_CXX)
    endif()
endif()

add_executable(verifier main_verifier.cpp)
target_link_libraries(verifier rapidsnarkStatic)

if((USE_LOGGER OR NOT USE_OPENMP) AND NOT TARGET_PLATFORM MATCHES "android")
    target_link_libraries(prover pthread)
    target_link_libraries(verifier pthread)
endif()

if(USE_SODIUM)
    target_link_libraries(prover sodium)
endif()

option(BUILD_TESTS "Build the tests" ON)

if(BUILD_TESTS)
    enable_testing()
    add_executable(test_public_size test_public_size.c)
    target_link_libraries(test_public_size rapidsnarkStaticFrFq pthread)
    add_test(NAME test_public_size COMMAND test_public_size circuit_final.zkey 86
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/testdata)
endif()

if(OpenMP_CXX_FOUND)
    if(TARGET_PLATFORM MATCHES "android")
        target_link_libraries(prover -static-openmp -fopenmp)
        target_link_libraries(verifier -static-openmp -fopenmp)
        target_link_libraries(rapidsnark -static-openmp -fopenmp)

    elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
        target_link_libraries(prover OpenMP::OpenMP_CXX)
        target_link_libraries(verifier OpenMP::OpenMP_CXX)
        if(BUILD_TESTS)
            target_link_libraries(test_public_size OpenMP::OpenMP_CXX)
        endif()
    endif()
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
        # Use static libraries to avoid runtime dylib dependencies
        target_link_libraries(proverServer rapidsnarkStaticFrFq ${CMAKE_SOURCE_DIR}/depends/pistache/build/src/libpistache.a ${LIB_EVENT_DIR}/libevent.a ${LIB_EVENT_DIR}/libevent_pthreads.a pthread)
    elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
        message("BUILD prover server for LINUX")
        target_link_directories(proverServer PRIVATE ${CMAKE_SOURCE_DIR}/depends/pistache/build/src)
        target_link_libraries(proverServer PRIVATE pistache rapidsnark OpenMP::OpenMP_CXX)
    endif()
endif()
