#ifndef COMPILER_H
#define COMPILER_H

#include "Instruction.h"
#include "Value.h"
#include "Lexer.h"
#include <vector>
#include <unordered_map>
#include <string>
#include <bitset>

struct Goto {
    std::string labelName;
    int instructionIndex;
};

struct UpvalueInfo {
    bool isLocal;
    int index;
};

struct Prototype {
    std::vector<Instruction> instructions;
    std::vector<Value> constants;
    std::vector<Prototype*> protos; // Nested functions
    std::vector<UpvalueInfo> upvalues;
    int numParams;
};

// Represents the state of the function currently being compiled
struct CompilerState {
    Prototype* proto;
    std::unordered_map<std::string, int> locals;
    std::unordered_map<std::string, int> labels; // label name -> pc
    std::vector<Goto> pendingGotos;
    std::vector<std::vector<int>> breakJumps; // Jumps to patch for break statements

    int nextReg;
    std::bitset<256> allocatedRegs;
    CompilerState* enclosing; // Parent scope

    CompilerState(CompilerState* parent) : proto(new Prototype()), nextReg(0), enclosing(parent) {
        proto->numParams = 0;
    }
};

class Compiler {
public:
    Compiler();
    Prototype* compile(const std::string& source); // Returns the main chunk prototype

private:
    std::vector<Token> tokens;
    int currentTokenIdx;

    CompilerState* current;

    Token peek();
    Token advance();
    bool match(TokenType type);
    Token consume(TokenType type, const std::string& errorMessage);

    void parseStatement();
    void parseStatementImpl();
    void parseIfStatement();
    void parseWhileStatement();
    void parseForStatement();
    void parseBreakStatement();
    void parseGotoStatement();
    void parseLabelStatement();
    void parseFunctionStatement();
    int parseFunctionExpression();
    void parseReturnStatement();
    void parseBlock();

    int parseExpression();
    int parseLogic();
    int parseComparison();
    int parseConcatenation();
    int parseTerm();
    int parseFactor();
    int parseUnary();
    int parseAtom();
    int parseTableConstructor();

    // Variable access
    void parseVariable(Token name, bool isAssignment, int rValueReg);

    // Upvalues
    int resolveLocal(CompilerState* state, const std::string& name);
    int resolveUpvalue(CompilerState* state, const std::string& name);
    int addUpvalue(CompilerState* state, int index, bool isLocal);

    std::vector<std::string> snapshotLocals();
    void restoreLocals(const std::vector<std::string>& snapshot);
    void resolveGotos();

    int addConstant(Value v);
    void emit(Instruction inst);
    int emitJump(OpCode op, int condReg = 0);
    void patchJump(int instructionIndex);
    int allocateRegister();
};

#endif
