const { sh, cli } = require("tasksfile");

function cleanAll() {
    sh("rm -rf build_nodejs");
}

function createFieldSources() {
    sh("mkdir -p build_nodejs");
    sh("npm install", {cwd: "depends/ffiasm"});
    sh("node ../depends/ffiasm/src/buildzqfield.js -q 21888242871839275222246405745257275088696311157297823662689037894645226208583 -n Fq", {cwd: "build_nodejs"});
    sh("node ../depends/ffiasm/src/buildzqfield.js -q 21888242871839275222246405745257275088548364400416034343698204186575808495617 -n Fr", {cwd: "build_nodejs"});

    if (process.platform === "darwin") {
        sh("nasm -fmacho64 --prefix _ fq.asm", {cwd: "build_nodejs"});
    }  else if (process.platform === "linux") {
        sh("nasm -felf64 fq.asm", {cwd: "build_nodejs"});
    } else throw("Unsupported platform");

    if (process.platform === "darwin") {
        sh("nasm -fmacho64 --prefix _ fr.asm", {cwd: "build_nodejs"});
    }  else if (process.platform === "linux") {
        sh("nasm -felf64 fr.asm", {cwd: "build_nodejs"});
    } else throw("Unsupported platform");
}

function buildPistache() {
    sh("git submodule init && git submodule update");
    sh("mkdir -p build", {cwd: "depends/pistache"});
    sh("cmake -G \"Unix Makefiles\" -DCMAKE_BUILD_TYPE=Release ..", {cwd: "depends/pistache/build"});
    sh("make", {cwd: "depends/pistache/build"});
}

function buildPistacheMac() {
    sh("mkdir -p build", {cwd: "depends/pistache"});
    sh("cmake -G \"Unix Makefiles\" -DCMAKE_BUILD_TYPE=Release -DPISTACHE_BUILD_TESTS=OFF -DLIB_EVENT_INCLUDE_DIR=/opt/homebrew/opt/libevent/include ..", {cwd: "depends/pistache/build"});
    sh("make", {cwd: "depends/pistache/build"});
}

function buildProverServer() {
    sh("g++" +
        " -I."+
        " -I../src"+
        " -I../depends/pistache/include"+
        " -I../depends/json/single_include"+
        " -I../depends/ffiasm/c"+
        " ../src/main_proofserver.cpp"+
        " ../src/proverapi.cpp"+
        " ../src/fullprover.cpp"+
        " ../src/binfile_utils.cpp"+
        " ../src/fileloader.cpp" +
        " ../src/wtns_utils.cpp"+
        " ../src/zkey_utils.cpp"+
        " ../src/logger.cpp"+
        " ../depends/ffiasm/c/misc.cpp"+
        " ../depends/ffiasm/c/naf.cpp"+
        " ../depends/ffiasm/c/splitparstr.cpp"+
        " ../depends/ffiasm/c/alt_bn128.cpp"+
        " fq.cpp"+
        " fq.o"+
        " fr.cpp"+
        " fr.o"+
        " -L../depends/pistache/build/src -lpistache"+
        " -o proverServer"+
        " -fmax-errors=5 -std=c++17 -DUSE_OPENMP -DUSE_ASM -DARCH_X86_64 -DUSE_LOGGER -pthread -lgmp -lsodium -fopenmp -O3", {cwd: "build", nopipe: true}
    );
}


function buildProverServerSingleThread() {
    sh("g++" +
        " -I."+
        " -I../src"+
        " -I../depends/pistache/include"+
        " -I../depends/json/single_include"+
        " -I../depends/ffiasm/c"+
        " ../src/main_proofserver.cpp"+
        " ../src/proverapi.cpp"+
        " ../src/fullprover.cpp"+
        " ../src/binfile_utils.cpp"+
        " ../src/fileloader.cpp" +
        " ../src/wtns_utils.cpp"+
        " ../src/zkey_utils.cpp"+
        " ../src/logger.cpp"+
        " ../depends/ffiasm/c/misc.cpp"+
        " ../depends/ffiasm/c/naf.cpp"+
        " ../depends/ffiasm/c/splitparstr.cpp"+
        " ../depends/ffiasm/c/alt_bn128.cpp"+
        " fq.cpp"+
        " fq.o"+
        " fr.cpp"+
        " fr.o"+
        " -L../depends/pistache/build/src -lpistache"+
        " -o proverServerSingleThread"+
        " -fmax-errors=5 -std=c++17 -DUSE_ASM -DARCH_X86_64 -DUSE_LOGGER -lgmp -lsodium -O3", {cwd: "build_nodejs", nopipe: true}
    );
}


function buildProver() {
    sh("g++" +
        " -I."+
        " -I../src"+
        " -I../depends/ffiasm/c"+
        " -I../depends/json/single_include"+
        " ../src/main_prover.cpp"+
        " ../src/binfile_utils.cpp"+
        " ../src/fileloader.cpp" +
        " ../src/prover.cpp"+
        " ../src/zkey_utils.cpp"+
        " ../src/wtns_utils.cpp"+
        " ../src/logger.cpp"+
        " ../depends/ffiasm/c/misc.cpp"+
        " ../depends/ffiasm/c/naf.cpp"+
        " ../depends/ffiasm/c/splitparstr.cpp"+
        " ../depends/ffiasm/c/alt_bn128.cpp"+
        " fq.cpp"+
        " fq.o"+
        " fr.cpp"+
        " fr.o"+
        " -o prover" +
        " -fmax-errors=5 -std=c++17 -DUSE_OPENMP -DUSE_ASM -DARCH_X86_64 -pthread -lgmp -lsodium -fopenmp -O3", {cwd: "build_nodejs", nopipe: true}
    );
}


cli({
    cleanAll,
    createFieldSources,
    buildPistache,
    buildPistacheMac,
    buildProverServer,
    buildProver,
    buildProverServerSingleThread
});
