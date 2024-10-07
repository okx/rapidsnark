cmake_minimum_required(VERSION 3.22.1)
set(CMAKE_CXX_STANDARD 17)
if(USE_CUDA)
    set(CMAKE_CUDA_STANDARD 17)
    set(CMAKE_CUDA_STANDARD_REQUIRED TRUE)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

include(cmake/platform.cmake)

set(USE_ASM    ON CACHE BOOL "Use asm implementation for Fr and Fq")
set(USE_OPENMP ON CACHE BOOL "Use OpenMP")
set(BUILD_SERVER OFF CACHE BOOL "Build Server mode")
set(G2_ENABLED OFF CACHE BOOL "enable G2 curve")

if(G2_ENABLED)
    add_definitions(-DG2_ENABLED)
endif()

if(USE_CUDA)
    project(rapidsnark LANGUAGES CXX CUDA ASM)
else()
    project(rapidsnark LANGUAGES CXX ASM)
endif()


message("TARGET_PLATFORM=" ${TARGET_PLATFORM})
message("USE_ASM=" ${USE_ASM})
message("USE_OPENMP=" ${USE_OPENMP})
message("CMAKE_CROSSCOMPILING=" ${CMAKE_CROSSCOMPILING})

message("GMP_PREFIX=" ${GMP_PREFIX})
message("GMP_INCLUDE_DIR=" ${GMP_INCLUDE_DIR})
message("GMP_LIB_DIR=" ${GMP_LIB_DIR})

message("LIB_OMP_PREFIX=" ${LIB_OMP_PREFIX})

if (NOT EXISTS ${GMP_INCLUDE_FILE_FULLPATH})
    message("WARNING: ${GMP_INCLUDE_FILE_FULLPATH} is not found and so system ${GMP_INCLUDE_FILE} is used.")
endif()

if (NOT EXISTS ${GMP_LIB_FILE_FULLPATH})
    message("WARNING: ${GMP_LIB_FILE_FULLPATH} is not found and so system ${GMP_LIB_FILE} is used.")
    set(GMP_LIB gmp)
endif()


if(USE_OPENMP)
    find_package(OpenMP)

    if(OpenMP_CXX_FOUND)
        if(TARGET_PLATFORM MATCHES "android")
            message("OpenMP is used")

        elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
            message("OpenMP is used")
        else()
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
            message("OpenMP is not used")

        endif()
    endif()
endif()


add_subdirectory(src)

if(BUILD_SERVER)
    install(TARGETS proverServer prover test_prover rapidsnark fr fq
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
    BUNDLE DESTINATION ${CMAKE_INSTALL_PREFIX}/app
    LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
elseif()
    install(TARGETS prover test_prover rapidsnark fr fq
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
    BUNDLE DESTINATION ${CMAKE_INSTALL_PREFIX}/app
    LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
endif()

install(FILES "${GMP_LIB_DIR}/${GMP_LIB_FILE}"
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)

install(FILES src/prover.h
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
