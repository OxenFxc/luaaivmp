#ifndef LUAGENERATOR_H
#define LUAGENERATOR_H

#include "Compiler.h"
#include "VMP/OpCodeStrategy.h"
#include <iostream>

class LuaGenerator {
public:
    static void generate(Prototype* proto, std::ostream& out, const OpCodeStrategy& strategy, bool pack = false, bool encrypt = false);
private:
    static void generateProto(Prototype* proto, std::ostream& out, int index, const OpCodeStrategy& strategy, bool encrypt);
};

#endif
