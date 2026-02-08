#ifndef LUAGENERATOR_H
#define LUAGENERATOR_H

#include "Compiler.h"
#include "VMP/OpCodeStrategy.h"
#include <iostream>

class LuaGenerator {
public:
    static void generate(Prototype* proto, std::ostream& out, const OpCodeStrategy& strategy);
private:
    static void generateProto(Prototype* proto, std::ostream& out, int index, const OpCodeStrategy& strategy);
};

#endif
