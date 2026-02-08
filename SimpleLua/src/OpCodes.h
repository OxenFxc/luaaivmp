#ifndef OPCODES_H
#define OPCODES_H

enum OpCode {
    OP_MOVE,    // R(A) := R(B)
    OP_LOADK,   // R(A) := K(Bx)
    OP_ADD,     // R(A) := R(B) + R(C)
    OP_SUB,     // R(A) := R(B) - R(C)
    OP_MUL,     // R(A) := R(B) * R(C)
    OP_DIV,     // R(A) := R(B) / R(C)
    OP_EQ,      // R(A) := (R(B) == R(C))
    OP_LT,      // R(A) := (R(B) < R(C))
    OP_LE,      // R(A) := (R(B) <= R(C))
    OP_JMP,     // PC := PC + B (Unconditional Jump)
    OP_JMP_FALSE, // PC := PC + B if not R(A)
    OP_GETGLOBAL, // R(A) := Gbl[K(B)]
    OP_SETGLOBAL, // Gbl[K(B)] := R(A)
    OP_NEWTABLE,  // R(A) := {}
    OP_GETTABLE,  // R(A) := R(B)[R(C)]
    OP_SETTABLE,  // R(A)[R(B)] := R(C)
    OP_CALL,      // R(A) ... := R(A)(R(A+1), ..., R(A+B-1))
    OP_CLOSURE,   // R(A) := closure(KPROTO[Bx])
    OP_GETUPVAL,  // R(A) := UpValue[B]
    OP_SETUPVAL,  // UpValue[B] := R(A)
    OP_PRINT,   // Custom: print(R(A))
    OP_RETURN   // return R(A) ... (or variable returns)
};

#endif
