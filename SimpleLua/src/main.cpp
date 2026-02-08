#include <iostream>
#include <fstream>
#include <string>
#include "Compiler.h"
#include "LuaGenerator.h"

int main() {
    std::string source =
        "function sum(...)\n"
        "    local s = 0;\n"
        "    local args = {...};\n"
        "    print(\"Number of args: \" .. #args);\n"
        "    local i = 1;\n"
        "    while i <= #args do\n"
        "        s = s + args[i];\n"
        "        i = i + 1;\n"
        "    end\n"
        "    return s;\n"
        "end\n"
        "\n"
        "local res = sum(1, 2, 3, 4, 5);\n"
        "print(\"Sum: \" .. res);\n"
        "\n"
        "if not (res > 100) then\n"
        "    print(\"Sum is not greater than 100\");\n"
        "end\n"
        "\n"
        "local is_even = (res % 2) == 0;\n"
        "print(\"Is even? \" .. tostring(is_even));"; // tostring might not be in _G access optimization yet? GETGLOBAL handles it.

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
