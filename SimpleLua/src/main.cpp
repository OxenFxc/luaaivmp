#include <iostream>
#include <fstream>
#include <string>
#include "Compiler.h"
#include "LuaGenerator.h"

int main() {
    std::string source =
        "function make_counter()\n"
        "    local count = 0;\n"
        "    function increment()\n"
        "        count = count + 1;\n"
        "        return count;\n"
        "    end\n"
        "    return increment;\n"
        "end\n"
        "\n"
        "local counter = make_counter();\n"
        "print(counter());\n"
        "print(counter());\n"
        "\n"
        "local t = {};\n"
        "local mt = {};\n"
        "mt.__call = function(t, x)\n"
        "    print(\"Called table with: \");\n"
        "    print(x);\n"
        "end;\n"
        "setmetatable(t, mt);\n"
        "t(123);\n"
        "\n"
        "print(\"Done\");";

    std::cout << "Source Code:\n" << source << "\n\n";

    try {
        std::cout << "Compiling...\n";
        Compiler compiler;
        Prototype* proto = compiler.compile(source);

        std::cout << "Compiled successfully.\n";
        std::cout << "Main Instructions: " << proto->instructions.size() << "\n";
        std::cout << "Constants: " << proto->constants.size() << "\n";
        std::cout << "Nested Functions: " << proto->protos.size() << "\n\n";

        std::cout << "Generating Lua VM code...\n";
        std::ofstream outFile("output.lua");
        if (!outFile) {
            std::cerr << "Error: Could not open output.lua for writing.\n";
            return 1;
        }
        LuaGenerator::generate(proto, outFile);
        outFile.close();

        std::cout << "Generated output.lua\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
