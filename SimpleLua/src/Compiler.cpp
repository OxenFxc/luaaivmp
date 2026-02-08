#include "Compiler.h"
#include <stdexcept>
#include <algorithm>

Compiler::Compiler() : currentTokenIdx(0), current(nullptr) {}

Prototype* Compiler::compile(const std::string& source) {
    Lexer lexer(source);
    tokens = lexer.tokenize();
    currentTokenIdx = 0;

    current = new CompilerState(nullptr);

    while (peek().type != TokenType::END_OF_FILE) {
        parseStatement();
    }

    resolveGotos();
    emit(Instruction(OP_RETURN, 0, 0, 0));
    return current->proto;
}

Token Compiler::peek() {
    return tokens[currentTokenIdx];
}

Token Compiler::advance() {
    if (currentTokenIdx < (int)tokens.size()) {
        return tokens[currentTokenIdx++];
    }
    return tokens.back();
}

bool Compiler::match(TokenType type) {
    if (peek().type == type) {
        advance();
        return true;
    }
    return false;
}

Token Compiler::consume(TokenType type, const std::string& errorMessage) {
    if (peek().type == type) {
        return advance();
    }
    throw std::runtime_error(errorMessage + ". Got: " + peek().value + " at line " + std::to_string(peek().line));
}

void Compiler::parseStatement() {
    parseStatementImpl();
    current->allocatedRegs.reset();
    for (const auto& kv : current->locals) {
        current->allocatedRegs[kv.second] = true;
    }
}

void Compiler::parseStatementImpl() {
    if (match(TokenType::LOCAL)) {
        if (match(TokenType::FUNCTION)) {
            Token name = consume(TokenType::ID, "Expect function name after 'local function'");
            current->locals[name.value] = allocateRegister();
            int varReg = current->locals[name.value];
            int funcReg = parseFunctionExpression();
            if (varReg != funcReg) {
                emit(Instruction(OP_MOVE, varReg, funcReg, 0));
            }
        } else {
            std::vector<std::string> names;
            do {
                names.push_back(consume(TokenType::ID, "Expect variable name after 'local'").value);
            } while (match(TokenType::COMMA));

            std::vector<int> exprRegs;
            if (match(TokenType::ASSIGN)) {
                do {
                    exprRegs.push_back(parseExpression());
                } while (match(TokenType::COMMA));
            }

            // Adjust results if last expression is a CALL and we need more values
            int needed = (int)names.size() - (int)exprRegs.size() + 1;
            if (needed > 1 && !exprRegs.empty()) {
                int lastExpr = exprRegs.back();
                // Check if last instruction was a CALL producing lastExpr
                if (current->proto->instructions.size() > 0) {
                     Instruction& last = current->proto->instructions.back();
                     if (last.op == OP_CALL && last.a == lastExpr && last.c == 2) {
                         last.c = needed + 1; // Request needed+1 results
                         // The CALL puts results in lastExpr, lastExpr+1, ...
                         // Add extra results to exprRegs
                         for (int i = 1; i < needed; ++i) {
                             exprRegs.push_back(lastExpr + i);
                             // Mark as allocated so local allocation doesn't overwrite them
                             current->allocatedRegs[lastExpr + i] = true;
                         }
                     }
                }
            }

            for (size_t i = 0; i < names.size(); ++i) {
                current->locals[names[i]] = allocateRegister();
                int varReg = current->locals[names[i]];

                if (i < exprRegs.size()) {
                    emit(Instruction(OP_MOVE, varReg, exprRegs[i], 0));
                } else {
                    int nilIdx = addConstant(Value(Nil{}));
                    int nilReg = allocateRegister();
                    emit(Instruction(OP_LOADK, nilReg, nilIdx));
                    emit(Instruction(OP_MOVE, varReg, nilReg, 0));
                }
            }
        }

        if (match(TokenType::SEMICOLON)) {}
    } else if (match(TokenType::IF)) {
        parseIfStatement();
    } else if (match(TokenType::WHILE)) {
        parseWhileStatement();
    } else if (match(TokenType::FOR)) {
        parseForStatement();
    } else if (match(TokenType::FUNCTION)) {
        parseFunctionStatement();
    } else if (match(TokenType::RETURN)) {
        parseReturnStatement();
    } else if (match(TokenType::GOTO)) {
        parseGotoStatement();
    } else if (match(TokenType::DOUBLE_COLON)) {
        parseLabelStatement();
    } else {
        Token t = peek();
        if (t.type == TokenType::ID) {
            advance();
            // Direct assignment: ID = expr
            if (match(TokenType::ASSIGN)) {
                int exprReg = parseExpression();
                parseVariable(t, true, exprReg);
                if (match(TokenType::SEMICOLON)) {}
                return;
            }

            // Prefix expression (l-value or call)
            int valReg;
            int localReg = resolveLocal(current, t.value);
            if (localReg != -1) {
                valReg = localReg;
            } else {
                int upvalIdx = resolveUpvalue(current, t.value);
                if (upvalIdx != -1) {
                    valReg = allocateRegister();
                    emit(Instruction(OP_GETUPVAL, valReg, upvalIdx, 0));
                } else {
                    valReg = allocateRegister();
                    int nameIdx = addConstant(t.value);
                    emit(Instruction(OP_GETGLOBAL, valReg, nameIdx));
                }
            }

            while (true) {
                if (match(TokenType::DOT)) {
                    Token key = consume(TokenType::ID, "Expect key");
                    if (match(TokenType::ASSIGN)) {
                        int rVal = parseExpression();
                        int keyIdx = addConstant(key.value);
                        int keyReg = allocateRegister();
                        emit(Instruction(OP_LOADK, keyReg, keyIdx));
                        emit(Instruction(OP_SETTABLE, valReg, keyReg, rVal));
                        if (match(TokenType::SEMICOLON)) {}
                        return;
                    }
                    int keyIdx = addConstant(key.value);
                    int keyReg = allocateRegister();
                    emit(Instruction(OP_LOADK, keyReg, keyIdx));
                    int resReg = allocateRegister();
                    emit(Instruction(OP_GETTABLE, resReg, valReg, keyReg));
                    valReg = resReg;
                } else if (match(TokenType::LBRACKET)) {
                    int keyReg = parseExpression();
                    consume(TokenType::RBRACKET, "Expect ']'");
                    if (match(TokenType::ASSIGN)) {
                        int rVal = parseExpression();
                        emit(Instruction(OP_SETTABLE, valReg, keyReg, rVal));
                        if (match(TokenType::SEMICOLON)) {}
                        return;
                    }
                    int resReg = allocateRegister();
                    emit(Instruction(OP_GETTABLE, resReg, valReg, keyReg));
                    valReg = resReg;
                } else if (match(TokenType::LPAREN)) {
                    std::vector<int> args;
                    if (!match(TokenType::RPAREN)) {
                        do {
                            args.push_back(parseExpression());
                        } while (match(TokenType::COMMA));
                        consume(TokenType::RPAREN, "Expect ')' after arguments");
                    }
                    int base = valReg;
                    for (size_t i = 0; i < args.size(); ++i) {
                        emit(Instruction(OP_MOVE, base + 1 + i, args[i], 0));
                    }
                    emit(Instruction(OP_CALL, base, args.size() + 1, 1));
                    if (match(TokenType::SEMICOLON)) {}
                    return;
                } else if (match(TokenType::COLON)) {
                    Token method = consume(TokenType::ID, "Expect method name");
                    consume(TokenType::LPAREN, "Expect '('");
                    int keyIdx = addConstant(method.value);
                    int keyReg = allocateRegister();
                    emit(Instruction(OP_LOADK, keyReg, keyIdx));

                    int funcReg = allocateRegister();
                    emit(Instruction(OP_GETTABLE, funcReg, valReg, keyReg));

                    std::vector<int> args;
                    args.push_back(valReg); // self
                    if (!match(TokenType::RPAREN)) {
                        do {
                            args.push_back(parseExpression());
                        } while (match(TokenType::COMMA));
                        consume(TokenType::RPAREN, "Expect ')'");
                    }

                    int base = funcReg;
                    for (size_t i = 0; i < args.size(); ++i) {
                        emit(Instruction(OP_MOVE, base + 1 + i, args[i], 0));
                    }
                    emit(Instruction(OP_CALL, base, args.size() + 1, 1));
                    if (match(TokenType::SEMICOLON)) {}
                    return;
                } else {
                    throw std::runtime_error("Unexpected token in statement: " + peek().value);
                }
            }
        }
        throw std::runtime_error("Unexpected token: " + peek().value);
    }
}

void Compiler::parseVariable(Token name, bool isAssignment, int rValueReg) {
    int localReg = resolveLocal(current, name.value);
    if (localReg != -1) {
        if (isAssignment) {
            emit(Instruction(OP_MOVE, localReg, rValueReg, 0));
        }
    } else {
        int upvalIdx = resolveUpvalue(current, name.value);
        if (upvalIdx != -1) {
            if (isAssignment) {
                emit(Instruction(OP_SETUPVAL, rValueReg, upvalIdx, 0));
            }
        } else {
            // Global
            int nameIdx = addConstant(name.value);
            if (isAssignment) {
                emit(Instruction(OP_SETGLOBAL, rValueReg, nameIdx));
            }
        }
    }
}

void Compiler::parseFunctionStatement() {
    Token name = consume(TokenType::ID, "Expect function name");
    consume(TokenType::LPAREN, "Expect '('");

    CompilerState* fnState = new CompilerState(current);
    current->proto->protos.push_back(fnState->proto);
    int protoIdx = (int)current->proto->protos.size() - 1;

    CompilerState* parent = current;
    current = fnState;

    if (!match(TokenType::RPAREN)) {
        do {
            if (match(TokenType::DOTDOTDOT)) {
                // Vararg found, must be last.
                // We should ensure we don't expect more commas?
                // But loop condition is match(COMMA).
                // If we hit ..., we break loop?
                // But we are inside do-while.
                // If I match ..., I should consume RPAREN and return.
                break;
            }
            Token param = consume(TokenType::ID, "Expect parameter name");
            current->locals[param.value] = allocateRegister();
            current->proto->numParams++;
        } while (match(TokenType::COMMA));

        consume(TokenType::RPAREN, "Expect ')' after parameters");
    }

    while (peek().type != TokenType::END && peek().type != TokenType::END_OF_FILE) {
        parseStatement();
    }
    consume(TokenType::END, "Expect 'end' after function body");

    emit(Instruction(OP_RETURN, 0, 1, 0));
    resolveGotos();

    current = parent;

    int reg = allocateRegister();
    emit(Instruction(OP_CLOSURE, reg, protoIdx));

    int nameIdx = addConstant(name.value);
    emit(Instruction(OP_SETGLOBAL, reg, nameIdx));
}

int Compiler::parseFunctionExpression() {
    consume(TokenType::LPAREN, "Expect '('");

    CompilerState* fnState = new CompilerState(current);
    current->proto->protos.push_back(fnState->proto);
    int protoIdx = (int)current->proto->protos.size() - 1;

    CompilerState* parent = current;
    current = fnState;

    if (!match(TokenType::RPAREN)) {
        do {
            if (match(TokenType::DOTDOTDOT)) {
                break;
            }
            Token param = consume(TokenType::ID, "Expect parameter name");
            current->locals[param.value] = allocateRegister();
            current->proto->numParams++;
        } while (match(TokenType::COMMA));
        consume(TokenType::RPAREN, "Expect ')' after parameters");
    }

    while (peek().type != TokenType::END && peek().type != TokenType::END_OF_FILE) {
        parseStatement();
    }
    consume(TokenType::END, "Expect 'end' after function body");

    emit(Instruction(OP_RETURN, 0, 1, 0));
    resolveGotos();

    current = parent;

    int reg = allocateRegister();
    emit(Instruction(OP_CLOSURE, reg, protoIdx));
    return reg;
}

void Compiler::parseReturnStatement() {
    if (peek().type == TokenType::SEMICOLON || peek().type == TokenType::END || peek().type == TokenType::ELSE) {
        emit(Instruction(OP_RETURN, 0, 1, 0));
    } else {
        std::vector<int> exprRegs;
        do {
            exprRegs.push_back(parseExpression());
        } while (match(TokenType::COMMA));

        int n = (int)exprRegs.size();
        int base = allocateRegister();
        for (int i = 1; i < n; ++i) allocateRegister();

        for (int i = 0; i < n; ++i) {
             emit(Instruction(OP_MOVE, base + i, exprRegs[i], 0));
        }
        emit(Instruction(OP_RETURN, base, n + 1, 0));
    }
    if (match(TokenType::SEMICOLON)) {}
}

std::vector<std::string> Compiler::snapshotLocals() {
    std::vector<std::string> names;
    names.reserve(current->locals.size());
    for (const auto& kv : current->locals) {
        names.push_back(kv.first);
    }
    return names;
}

void Compiler::restoreLocals(const std::vector<std::string>& snapshot) {
    auto it = current->locals.begin();
    while (it != current->locals.end()) {
        bool found = false;
        for (const auto& name : snapshot) {
            if (name == it->first) {
                found = true;
                break;
            }
        }
        if (!found) {
            current->allocatedRegs[it->second] = false;
            it = current->locals.erase(it);
        } else {
            ++it;
        }
    }
}

void Compiler::parseIfStatement() {
    int condReg = parseExpression();
    consume(TokenType::THEN, "Expect 'then' after condition");

    int jumpFalse = emitJump(OP_JMP_FALSE, condReg);

    std::vector<std::string> snapshot = snapshotLocals();
    while (peek().type != TokenType::ELSEIF && peek().type != TokenType::ELSE && peek().type != TokenType::END && peek().type != TokenType::END_OF_FILE) {
        parseStatement();
    }
    restoreLocals(snapshot);

    std::vector<int> jumpEnds;
    jumpEnds.push_back(emitJump(OP_JMP));
    patchJump(jumpFalse);

    while (match(TokenType::ELSEIF)) {
         int cond = parseExpression();
         consume(TokenType::THEN, "Expect 'then'");
         int jmpF = emitJump(OP_JMP_FALSE, cond);

         std::vector<std::string> loopSnapshot = snapshotLocals();
         while (peek().type != TokenType::ELSEIF && peek().type != TokenType::ELSE && peek().type != TokenType::END && peek().type != TokenType::END_OF_FILE) {
             parseStatement();
         }
         restoreLocals(loopSnapshot);

         jumpEnds.push_back(emitJump(OP_JMP));
         patchJump(jmpF);
    }

    if (match(TokenType::ELSE)) {
        std::vector<std::string> elseSnapshot = snapshotLocals();
        while (peek().type != TokenType::END && peek().type != TokenType::END_OF_FILE) {
            parseStatement();
        }
        restoreLocals(elseSnapshot);
    }

    consume(TokenType::END, "Expect 'end' after if statement");
    for (int j : jumpEnds) patchJump(j);
}

void Compiler::parseWhileStatement() {
    int loopStart = (int)current->proto->instructions.size();

    int condReg = parseExpression();
    consume(TokenType::DO, "Expect 'do' after while condition");

    int jumpFalse = emitJump(OP_JMP_FALSE, condReg);

    std::vector<std::string> snapshot = snapshotLocals();
    while (peek().type != TokenType::END && peek().type != TokenType::END_OF_FILE) {
        parseStatement();
    }
    restoreLocals(snapshot);

    emit(Instruction(OP_JMP, 0, loopStart - (int)current->proto->instructions.size() - 1));

    patchJump(jumpFalse);
    consume(TokenType::END, "Expect 'end' after while loop");
}

void Compiler::parseForStatement() {
    Token name = consume(TokenType::ID, "Expect variable name after 'for'");

    if (match(TokenType::ASSIGN)) {
        // Numeric for
        int startReg = parseExpression();
        consume(TokenType::COMMA, "Expect ',' after start value");
        int limitReg = parseExpression();

        int stepReg;
        if (match(TokenType::COMMA)) {
            stepReg = parseExpression();
        } else {
            stepReg = allocateRegister();
            int oneIdx = addConstant(1.0);
            emit(Instruction(OP_LOADK, stepReg, oneIdx));
        }

        consume(TokenType::DO, "Expect 'do' after for parameters");

        int base = allocateRegister(); // index
        allocateRegister(); // limit
        allocateRegister(); // step
        int varReg = allocateRegister(); // external variable

        // Lock registers for loop duration
        current->locals["(base " + std::to_string(base) + ")"] = base;
        current->locals["(limit " + std::to_string(base) + ")"] = base + 1;
        current->locals["(step " + std::to_string(base) + ")"] = base + 2;

        emit(Instruction(OP_MOVE, base, startReg, 0));
        emit(Instruction(OP_MOVE, base + 1, limitReg, 0));
        emit(Instruction(OP_MOVE, base + 2, stepReg, 0));

        int oldReg = -1;
        bool hadOld = false;
        if (current->locals.count(name.value)) {
            oldReg = current->locals[name.value];
            hadOld = true;
        }
        current->locals[name.value] = varReg;

        int loopStart = (int)current->proto->instructions.size();
        emit(Instruction(OP_FORPREP, base, 0));

        std::vector<std::string> snapshot = snapshotLocals();
        while (peek().type != TokenType::END && peek().type != TokenType::END_OF_FILE) {
            parseStatement();
        }
        restoreLocals(snapshot);

        consume(TokenType::END, "Expect 'end' after for loop");

        int loopEnd = (int)current->proto->instructions.size();
        emit(Instruction(OP_FORLOOP, base, 0));

        int prepOffset = loopEnd - loopStart - 1;
        current->proto->instructions[loopStart].b = prepOffset;

        int loopOffset = loopStart - loopEnd;
        current->proto->instructions[loopEnd].b = loopOffset;

        if (hadOld) {
            current->locals[name.value] = oldReg;
        } else {
            current->allocatedRegs[current->locals[name.value]] = false;
            current->locals.erase(name.value);
        }
        if (hadOld) current->allocatedRegs[varReg] = false;

        current->allocatedRegs[base] = false;
        current->allocatedRegs[base + 1] = false;
        current->allocatedRegs[base + 2] = false;
        current->locals.erase("(base " + std::to_string(base) + ")");
        current->locals.erase("(limit " + std::to_string(base) + ")");
        current->locals.erase("(step " + std::to_string(base) + ")");
    } else {
        // Generic for
        std::vector<std::string> varNames;
        varNames.push_back(name.value);
        while (match(TokenType::COMMA)) {
            varNames.push_back(consume(TokenType::ID, "Expect variable name").value);
        }
        consume(TokenType::IN, "Expect 'in' after variable list");

        int base = allocateRegister(); // iterator
        allocateRegister(); // state
        allocateRegister(); // control

        // Parse explist (expecting 3 values: iterator, state, control)
        int firstExpr = parseExpression();
        emit(Instruction(OP_MOVE, base, firstExpr, 0));

        bool patchedCall = false;
        if (!match(TokenType::COMMA)) {
            // Check if we can patch previous CALL
            if (current->proto->instructions.size() > 1) {
                Instruction& prev = current->proto->instructions[current->proto->instructions.size() - 2];
                if (prev.op == OP_CALL && prev.a == firstExpr && prev.c == 2) {
                    prev.c = 4; // 3 results
                    emit(Instruction(OP_MOVE, base + 1, firstExpr + 1, 0));
                    emit(Instruction(OP_MOVE, base + 2, firstExpr + 2, 0));
                    patchedCall = true;
                }
            }
        } else {
            int second = parseExpression();
            emit(Instruction(OP_MOVE, base + 1, second, 0));
            if (match(TokenType::COMMA)) {
                int third = parseExpression();
                emit(Instruction(OP_MOVE, base + 2, third, 0));
            } else {
                int nilIdx = addConstant(Value(Nil{}));
                int nilReg = allocateRegister();
                emit(Instruction(OP_LOADK, nilReg, nilIdx));
                emit(Instruction(OP_MOVE, base + 2, nilReg, 0));
            }
        }

        if (!patchedCall && current->proto->instructions.back().op == OP_MOVE && current->proto->instructions.back().b == base) {
             // If we didn't patch call and only had 1 expr, we need to set base+1, base+2 to nil?
             // Not strictly required if user knows what they are doing, but safe to init.
             // We'll skip for now to keep it simple.
        }

        // Cleanup registers used by explist so loop variables start immediately after control variable
        for (int r = base + 3; r < 256; ++r) {
            current->allocatedRegs[r] = false;
        }

        consume(TokenType::DO, "Expect 'do'");

        // Allocate registers for loop variables
        std::vector<int> loopVars;
        for (size_t i = 0; i < varNames.size(); ++i) {
            int r = allocateRegister();
            loopVars.push_back(r);
            current->locals[varNames[i]] = r;
        }

        int jumpInst = emitJump(OP_JMP);
        int loopStart = (int)current->proto->instructions.size();

        std::vector<std::string> snapshot = snapshotLocals();
        while (peek().type != TokenType::END && peek().type != TokenType::END_OF_FILE) {
            parseStatement();
        }
        restoreLocals(snapshot);

        consume(TokenType::END, "Expect 'end'");

        patchJump(jumpInst);

        emit(Instruction(OP_TFORCALL, base, 0, (int)varNames.size()));
        emit(Instruction(OP_TFORLOOP, base + 2, 0, 0));
        current->proto->instructions.back().b = loopStart - (int)current->proto->instructions.size();

        // Cleanup
        current->allocatedRegs[base] = false;
        current->allocatedRegs[base + 1] = false;
        current->allocatedRegs[base + 2] = false;
        for (int r : loopVars) current->allocatedRegs[r] = false;
    }
}

void Compiler::parseGotoStatement() {
    Token label = consume(TokenType::ID, "Expect label name after 'goto'");
    int inst = emitJump(OP_JMP);
    current->pendingGotos.push_back({label.value, inst});
    if (match(TokenType::SEMICOLON)) {}
}

void Compiler::parseLabelStatement() {
    Token label = consume(TokenType::ID, "Expect label name");
    consume(TokenType::DOUBLE_COLON, "Expect '::' after label name");
    if (current->labels.count(label.value)) {
        throw std::runtime_error("Label already defined: " + label.value);
    }
    current->labels[label.value] = (int)current->proto->instructions.size();
}

void Compiler::resolveGotos() {
    for (const auto& g : current->pendingGotos) {
        if (current->labels.count(g.labelName)) {
            int target = current->labels[g.labelName];
            int offset = target - g.instructionIndex - 1;
            current->proto->instructions[g.instructionIndex].b = offset;
        } else {
            throw std::runtime_error("Label not found: " + g.labelName);
        }
    }
    current->pendingGotos.clear();
}

int Compiler::parseExpression() {
    return parseLogic();
}

int Compiler::parseLogic() {
    int leftReg = parseComparison();

    while (peek().type == TokenType::AND || peek().type == TokenType::OR) {
        TokenType op = advance().type;
        // Short-circuit logic implementation using Jumps
        // A and B: if A is false, jump to end (result is A), else result is B
        // A or B: if A is true, jump to end (result is A), else result is B

        // This requires 'TESTSET' or similar, or manual JMPs.
        // Simplified: Evaluate both (no short circuit) or implement simple jumps.
        // Let's implement full short-circuit logic?
        // It requires patching.

        // For simplicity in this demo, I will treat AND/OR as binary operators without short-circuit
        // OR I can use the existing infrastructure.
        // But Lua AND/OR returns the value, not bool.

        // Let's defer proper short-circuit implementation and just parse the structure.
        // I don't have logical opcodes (OP_AND/OP_OR) in my VM set.
        // Using JMP_FALSE etc.

        // "and":
        //   res = left
        //   JMP_FALSE res, done
        //   right = parseComparison()
        //   res = right
        // done:

        int resReg = allocateRegister();
        emit(Instruction(OP_MOVE, resReg, leftReg, 0));

        if (op == TokenType::AND) {
            int jmp = emitJump(OP_JMP_FALSE, resReg);
            int rightReg = parseComparison();
            emit(Instruction(OP_MOVE, resReg, rightReg, 0));
            patchJump(jmp);
        } else {
            // "or"
            //   res = left
            //   JMP_TRUE res, done (Need JMP_TRUE or NOT + JMP_FALSE)
            //   No JMP_TRUE.
            //   Emulate:
            //   TEST res (if true skip next)
            //   JMP over_move
            //   JMP done
            // over_move:
            //   right = parseComparison
            //   res = right
            // done:

            // Or simpler if I add OP_TEST / OP_TESTSET which I haven't.
            // I added OP_NOT.
            // if not res then ... else jump done

            int notReg = allocateRegister();
            emit(Instruction(OP_NOT, notReg, resReg, 0));
            int jmp = emitJump(OP_JMP_FALSE, notReg); // Jump if NOT(res) is false (meaning res is true)

            int rightReg = parseComparison();
            emit(Instruction(OP_MOVE, resReg, rightReg, 0));

            patchJump(jmp);
        }
        leftReg = resReg;
    }
    return leftReg;
}

int Compiler::parseComparison() {
    int leftReg = parseConcatenation();

    while (peek().type == TokenType::EQ || peek().type == TokenType::NE ||
           peek().type == TokenType::LT || peek().type == TokenType::LE ||
           peek().type == TokenType::GT || peek().type == TokenType::GE) {
        TokenType op = advance().type;
        int rightReg = parseConcatenation();
        int resultReg = allocateRegister();

        if (op == TokenType::EQ) {
            emit(Instruction(OP_EQ, resultReg, leftReg, rightReg));
        } else if (op == TokenType::LT) {
            emit(Instruction(OP_LT, resultReg, leftReg, rightReg));
        } else if (op == TokenType::LE) {
            emit(Instruction(OP_LE, resultReg, leftReg, rightReg));
        } else if (op == TokenType::GT) {
            emit(Instruction(OP_LT, resultReg, rightReg, leftReg));
        } else if (op == TokenType::GE) {
            emit(Instruction(OP_LE, resultReg, rightReg, leftReg));
        } else {
             throw std::runtime_error("Operator not implemented yet");
        }
        leftReg = resultReg;
    }
    return leftReg;
}

int Compiler::parseConcatenation() {
    int leftReg = parseTerm();

    // Right associative ..
    if (match(TokenType::DOTDOT)) {
        int rightReg = parseConcatenation();
        int resultReg = allocateRegister();
        emit(Instruction(OP_CONCAT, resultReg, leftReg, rightReg));
        return resultReg;
    }
    return leftReg;
}

int Compiler::parseTerm() {
    int leftReg = parseFactor();
    while (peek().type == TokenType::PLUS || peek().type == TokenType::MINUS) {
        TokenType op = advance().type;
        int rightReg = parseFactor();
        int resultReg = allocateRegister();
        if (op == TokenType::PLUS) {
            emit(Instruction(OP_ADD, resultReg, leftReg, rightReg));
        } else {
            emit(Instruction(OP_SUB, resultReg, leftReg, rightReg));
        }
        leftReg = resultReg;
    }
    return leftReg;
}

int Compiler::parseFactor() {
    int leftReg = parseUnary();
    while (peek().type == TokenType::MUL || peek().type == TokenType::DIV || peek().type == TokenType::PERCENT || peek().type == TokenType::IDIV) {
        TokenType op = advance().type;
        int rightReg = parseUnary();
        int resultReg = allocateRegister();
        if (op == TokenType::MUL) {
            emit(Instruction(OP_MUL, resultReg, leftReg, rightReg));
        } else if (op == TokenType::DIV) {
            emit(Instruction(OP_DIV, resultReg, leftReg, rightReg));
        } else if (op == TokenType::IDIV) {
            emit(Instruction(OP_IDIV, resultReg, leftReg, rightReg));
        } else {
            emit(Instruction(OP_MOD, resultReg, leftReg, rightReg));
        }
        leftReg = resultReg;
    }
    return leftReg;
}

int Compiler::parseUnary() {
    if (match(TokenType::NOT)) {
        int operand = parseUnary();
        int reg = allocateRegister();
        emit(Instruction(OP_NOT, reg, operand, 0));
        return reg;
    } else if (match(TokenType::HASH)) {
        int operand = parseUnary();
        int reg = allocateRegister();
        emit(Instruction(OP_LEN, reg, operand, 0));
        return reg;
    } else if (match(TokenType::MINUS)) {
        // Unary minus: 0 - operand
        int operand = parseUnary();
        int reg = allocateRegister();
        int zeroIdx = addConstant(0.0);
        int zeroReg = allocateRegister();
        emit(Instruction(OP_LOADK, zeroReg, zeroIdx));
        emit(Instruction(OP_SUB, reg, zeroReg, operand));
        return reg;
    }
    return parseAtom();
}

int Compiler::parseAtom() {
    Token t = peek();
    if (match(TokenType::NUMBER)) {
        double val = std::stod(t.value);
        int constIdx = addConstant(val);
        int reg = allocateRegister();
        emit(Instruction(OP_LOADK, reg, constIdx));
        return reg;
    } else if (match(TokenType::STRING)) {
        int constIdx = addConstant(t.value);
        int reg = allocateRegister();
        emit(Instruction(OP_LOADK, reg, constIdx));
        return reg;
    } else if (match(TokenType::NIL)) {
        int constIdx = addConstant(Value(Nil{}));
        int reg = allocateRegister();
        emit(Instruction(OP_LOADK, reg, constIdx));
        return reg;
    } else if (match(TokenType::TRUE)) {
        int constIdx = addConstant(true);
        int reg = allocateRegister();
        emit(Instruction(OP_LOADK, reg, constIdx));
        return reg;
    } else if (match(TokenType::FALSE)) {
        int constIdx = addConstant(false);
        int reg = allocateRegister();
        emit(Instruction(OP_LOADK, reg, constIdx));
        return reg;
    } else if (match(TokenType::DOTDOTDOT)) {
        int reg = allocateRegister();
        emit(Instruction(OP_VARARG, reg, 0, 0)); // B=0 means "all", C=0 means "all results to top" (but register based...)
        // Simplified: C=2 means 1 result. C=0 means variable results.
        // For atomic expression, we usually want 1 value.
        // But `...` can return multiple.
        // Let's assume 1 for now or special handling.
        // Instruction OP_VARARG A B C: R(A), ..., R(A+C-2) = vararg
        // For expr, we just load into 1 reg?
        // Let's set C=2 (1 result)
        emit(Instruction(OP_VARARG, reg, 0, 2));
        return reg;
    } else if (match(TokenType::LBRACE)) {
        return parseTableConstructor();
    } else if (match(TokenType::ID)) {
        int valReg;

        // Resolve variable
        int localReg = resolveLocal(current, t.value);
        if (localReg != -1) {
            valReg = localReg;
        } else {
            int upvalIdx = resolveUpvalue(current, t.value);
            if (upvalIdx != -1) {
                valReg = allocateRegister();
                emit(Instruction(OP_GETUPVAL, valReg, upvalIdx, 0));
            } else {
                valReg = allocateRegister();
                int nameIdx = addConstant(t.value);
                emit(Instruction(OP_GETGLOBAL, valReg, nameIdx));
            }
        }

        while (true) {
            if (match(TokenType::DOT)) {
                Token key = consume(TokenType::ID, "Expect property name");
                int keyIdx = addConstant(key.value);
                int keyReg = allocateRegister();
                emit(Instruction(OP_LOADK, keyReg, keyIdx));

                int resReg = allocateRegister();
                emit(Instruction(OP_GETTABLE, resReg, valReg, keyReg));
                valReg = resReg;
            } else if (match(TokenType::LBRACKET)) {
                 int keyReg = parseExpression();
                 consume(TokenType::RBRACKET, "Expect ']'");
                 int resReg = allocateRegister();
                 emit(Instruction(OP_GETTABLE, resReg, valReg, keyReg));
                 valReg = resReg;
            } else if (match(TokenType::LPAREN)) {
                std::vector<int> args;
                if (!match(TokenType::RPAREN)) {
                    do {
                        args.push_back(parseExpression());
                    } while (match(TokenType::COMMA));
                    consume(TokenType::RPAREN, "Expect ')'");
                }

                int base = valReg;
                for(size_t i=0; i<args.size(); ++i) {
                     emit(Instruction(OP_MOVE, base + 1 + i, args[i], 0));
                }
                emit(Instruction(OP_CALL, base, args.size() + 1, 2));
            } else if (match(TokenType::COLON)) {
                Token method = consume(TokenType::ID, "Expect method name");
                int keyIdx = addConstant(method.value);
                int keyReg = allocateRegister();
                emit(Instruction(OP_LOADK, keyReg, keyIdx));

                int funcReg = allocateRegister();
                emit(Instruction(OP_GETTABLE, funcReg, valReg, keyReg));

                std::vector<int> args;
                args.push_back(valReg);

                consume(TokenType::LPAREN, "Expect '(' after method name");
                if (!match(TokenType::RPAREN)) {
                    do {
                        args.push_back(parseExpression());
                    } while (match(TokenType::COMMA));
                    consume(TokenType::RPAREN, "Expect ')'");
                }

                int base = funcReg;
                for(size_t i=0; i<args.size(); ++i) {
                     emit(Instruction(OP_MOVE, base + 1 + i, args[i], 0));
                }
                emit(Instruction(OP_CALL, base, args.size() + 1, 2));
                valReg = funcReg;
            } else {
                break;
            }
        }
        return valReg;

    } else if (match(TokenType::LPAREN)) {
        int reg = parseExpression();
        consume(TokenType::RPAREN, "Expect ')' after expression");
        return reg;
    } else if (match(TokenType::FUNCTION)) {
        return parseFunctionExpression();
    }

    throw std::runtime_error("Unexpected token in expression: " + t.value);
}

int Compiler::parseTableConstructor() {
    int tableReg = allocateRegister();
    emit(Instruction(OP_NEWTABLE, tableReg, 0, 0));

    if (peek().type == TokenType::RBRACE) {
        consume(TokenType::RBRACE, "Expect '}'");
        return tableReg;
    }

    int arrayIdx = 1;

    // Snapshot allocated registers to reuse them for each element
    std::bitset<256> snapshot = current->allocatedRegs;

    do {
        if (peek().type == TokenType::RBRACE) break;

        // Reset registers (except tableReg and previous allocations)
        current->allocatedRegs = snapshot;
        current->allocatedRegs[tableReg] = true;

        if (match(TokenType::LBRACKET)) {
             int keyReg = parseExpression();
             consume(TokenType::RBRACKET, "Expect ']'");
             consume(TokenType::ASSIGN, "Expect '='");
             int valReg = parseExpression();
             emit(Instruction(OP_SETTABLE, tableReg, keyReg, valReg));
        } else if (peek().type == TokenType::ID) {
             Token t = peek();

             advance();
             if (match(TokenType::ASSIGN)) {
                 int valReg = parseExpression();
                 int keyIdx = addConstant(t.value);
                 int keyReg = allocateRegister();
                 emit(Instruction(OP_LOADK, keyReg, keyIdx));
                 emit(Instruction(OP_SETTABLE, tableReg, keyReg, valReg));
             } else {
                 currentTokenIdx--;
                 int valReg = parseExpression();
                 int keyIdx = addConstant((double)arrayIdx++);
                 int keyReg = allocateRegister();
                 emit(Instruction(OP_LOADK, keyReg, keyIdx));
                 emit(Instruction(OP_SETTABLE, tableReg, keyReg, valReg));
             }
        } else {
            int valReg = parseExpression();
            int keyIdx = addConstant((double)arrayIdx++);
            int keyReg = allocateRegister();
            emit(Instruction(OP_LOADK, keyReg, keyIdx));
            emit(Instruction(OP_SETTABLE, tableReg, keyReg, valReg));
        }
    } while (match(TokenType::COMMA));
    consume(TokenType::RBRACE, "Expect '}'");
    return tableReg;
}

int Compiler::resolveLocal(CompilerState* state, const std::string& name) {
    if (state->locals.count(name)) {
        return state->locals[name];
    }
    return -1;
}

// Fixed logic for upvalues:
// CompilerState* state is the function trying to capture.
// We need to find `name` in `state->enclosing`.
int Compiler::resolveUpvalue(CompilerState* state, const std::string& name) {
    if (state->enclosing == nullptr) return -1;

    // 1. Is it a local in immediate parent?
    int local = resolveLocal(state->enclosing, name);
    if (local != -1) {
        return addUpvalue(state, local, true);
    }

    // 2. Is it an upvalue in immediate parent?
    int upvalue = resolveUpvalue(state->enclosing, name);
    if (upvalue != -1) {
        return addUpvalue(state, upvalue, false);
    }

    return -1;
}

int Compiler::addUpvalue(CompilerState* state, int index, bool isLocal) {
    for (size_t i = 0; i < state->proto->upvalues.size(); ++i) {
        if (state->proto->upvalues[i].index == index && state->proto->upvalues[i].isLocal == isLocal) {
            return i;
        }
    }
    state->proto->upvalues.push_back({isLocal, index});
    return state->proto->upvalues.size() - 1;
}

int Compiler::addConstant(Value v) {
    current->proto->constants.push_back(v);
    return current->proto->constants.size() - 1;
}

void Compiler::emit(Instruction inst) {
    current->proto->instructions.push_back(inst);
}

int Compiler::emitJump(OpCode op, int condReg) {
    emit(Instruction(op, condReg, 0));
    return current->proto->instructions.size() - 1;
}

void Compiler::patchJump(int instructionIndex) {
    int offset = (int)current->proto->instructions.size() - instructionIndex - 1;
    current->proto->instructions[instructionIndex].b = offset;
}

int Compiler::allocateRegister() {
    for (int i = 0; i < 256; ++i) {
        if (!current->allocatedRegs[i]) {
            current->allocatedRegs[i] = true;
            return i;
        }
    }
    throw std::runtime_error("Stack overflow: too many registers used");
}
