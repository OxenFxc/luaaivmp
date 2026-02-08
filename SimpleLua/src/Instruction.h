#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include "OpCodes.h"

struct Instruction {
    OpCode op;
    int a;
    int b;
    int c;

    Instruction(OpCode op, int a, int b, int c) : op(op), a(a), b(b), c(c) {}
    Instruction(OpCode op, int a, int bx) : op(op), a(a), b(bx), c(0) {} // For LOADK
};

#endif
