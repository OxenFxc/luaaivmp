#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <memory>
#include <cstring>
#include "Compiler.h"
#include "LuaGenerator.h"
#include "VMP/OpCodeStrategy.h"

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <input_file> <output_file> [-vmp]\n";
        return 1;
    }

    std::string inputPath = argv[1];
    std::string outputPath = argv[2];
    bool useVMP = false;

    for (int i = 3; i < argc; ++i) {
        if (std::strcmp(argv[i], "-vmp") == 0) {
            useVMP = true;
        }
    }

    std::ifstream inFile(inputPath);
    if (!inFile) {
        std::cerr << "Error: Could not open input file: " << inputPath << "\n";
        return 1;
    }

    std::stringstream buffer;
    buffer << inFile.rdbuf();
    std::string source = buffer.str();
    inFile.close();

    std::cout << "Compiling " << inputPath << "...\n";

    try {
        Compiler compiler;
        Prototype* proto = compiler.compile(source);

        std::cout << "Compiled successfully.\n";
        std::cout << "Main Instructions: " << proto->instructions.size() << "\n";
        std::cout << "Constants: " << proto->constants.size() << "\n";
        std::cout << "Nested Functions: " << proto->protos.size() << "\n\n";

        std::cout << "Generating Lua VM code to " << outputPath << "...\n";
        std::ofstream outFile(outputPath);
        if (!outFile) {
            std::cerr << "Error: Could not open output file for writing: " << outputPath << "\n";
            return 1;
        }

        std::unique_ptr<OpCodeStrategy> strategy;
        if (useVMP) {
            strategy = std::make_unique<RandomizedStrategy>();
            std::cout << "Using VMP (OpCode Randomization).\n";
        } else {
            strategy = std::make_unique<DefaultStrategy>();
        }

        LuaGenerator::generate(proto, outFile, *strategy);
        outFile.close();

        std::cout << "Generated " << outputPath << "\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
