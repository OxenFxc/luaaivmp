#ifndef LUAGENERATOR_H
#define LUAGENERATOR_H

#include "Compiler.h"
#include <iostream>

class LuaGenerator {
public:
    static void generate(Prototype* proto, std::ostream& out);
private:
    static void generateProto(Prototype* proto, std::ostream& out, int index);
};

#endif
