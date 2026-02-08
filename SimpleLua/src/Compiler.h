#ifndef COMPILER_H
#define COMPILER_H

#include "Instruction.h"
#include "Value.h"
#include "Lexer.h"
#include <vector>
#include <unordered_map>
#include <string>

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
    int nextReg;
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
    void parseIfStatement();
    void parseWhileStatement();
    void parseForStatement();
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

    int addConstant(Value v);
    void emit(Instruction inst);
    int emitJump(OpCode op, int condReg = 0);
    void patchJump(int instructionIndex);
    int allocateRegister();
};

#endif
