#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include "Compiler.h"
#include "LuaGenerator.h"

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <input_file> <output_file>\n";
        return 1;
    }

    std::string inputPath = argv[1];
    std::string outputPath = argv[2];

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
        LuaGenerator::generate(proto, outFile);
        outFile.close();

        std::cout << "Generated " << outputPath << "\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
