#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <stdexcept>
#include "prover.h"
<<<<<<< HEAD
#include "logger.hpp"

using namespace CPlusPlusLogging;
#define handle_error(msg) \
           do { perror(msg); exit(EXIT_FAILURE); } while (0)


const size_t BufferSize = 16384;

=======
#include "fileloader.hpp"
>>>>>>> upstream/main


int main(int argc, char **argv)
{
    if (argc != 5) {
        std::cerr << "Invalid number of parameters" << std::endl;
        std::cerr << "Usage: prover <circuit.zkey> <witness.wtns> <proof.json> <public.json>" << std::endl;
        return EXIT_FAILURE;
    }

    Logger::getInstance()->enableConsoleLogging();
    Logger::getInstance()->updateLogLevel(LOG_LEVEL_DEBUG);
    Logger::getInstance()->enableFileLogging();
    #if defined(USE_CUDA)
        LOG_INFO("start run prover in standalone mode, with GPU support ...");
    #else
        LOG_INFO("start run prover in standalone mode...");
    #endif

    try {
        const std::string zkeyFilename = argv[1];
        const std::string wtnsFilename = argv[2];
        const std::string proofFilename = argv[3];
        const std::string publicFilename = argv[4];

        BinFileUtils::FileLoader zkeyFile(zkeyFilename);
        BinFileUtils::FileLoader wtnsFile(wtnsFilename);
        std::vector<char>        publicBuffer;
        std::vector<char>        proofBuffer;
        unsigned long long       publicSize = 0;
        unsigned long long       proofSize = 0;
        char                     errorMsg[1024];

        int error = groth16_public_size_for_zkey_buf(
                     zkeyFile.dataBuffer(),
                     zkeyFile.dataSize(),
                     &publicSize,
                     errorMsg,
                     sizeof(errorMsg));

        if (error != PROVER_OK) {
            throw std::runtime_error(errorMsg);
        }

        groth16_proof_size(&proofSize);

        publicBuffer.resize(publicSize);
        proofBuffer.resize(proofSize);

        error = groth16_prover(
                   zkeyFile.dataBuffer(),
                   zkeyFile.dataSize(),
                   wtnsFile.dataBuffer(),
                   wtnsFile.dataSize(),
                   proofBuffer.data(),
                   &proofSize,
                   publicBuffer.data(),
                   &publicSize,
                   errorMsg,
                   sizeof(errorMsg));

        if (error != PROVER_OK) {
            throw std::runtime_error(errorMsg);
        }

        std::ofstream proofFile(proofFilename);
        proofFile.write(proofBuffer.data(), proofSize);

        std::ofstream publicFile(publicFilename);
        publicFile.write(publicBuffer.data(), publicSize);

    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;

    }

    exit(EXIT_SUCCESS);
}
