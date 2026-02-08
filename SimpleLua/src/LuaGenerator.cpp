#include "LuaGenerator.h"
#include <vector>
#include <sstream>
#include <iomanip>
#include <cctype>

// Helper prototypes
static std::string encryptString(const std::string& s);
static std::string encryptInstruction(int op, int a, int b, int c, int pc);
static std::string minify(std::string code);

void LuaGenerator::generate(Prototype* proto, std::ostream& out, const OpCodeStrategy& strategy, bool pack, bool encrypt) {
    std::stringstream ss;

    // 1. Opcodes definitions
    ss << "local OP_MOVE = " << strategy.get(OP_MOVE) << "\n";
    ss << "local OP_LOADK = " << strategy.get(OP_LOADK) << "\n";
    ss << "local OP_ADD = " << strategy.get(OP_ADD) << "\n";
    ss << "local OP_SUB = " << strategy.get(OP_SUB) << "\n";
    ss << "local OP_MUL = " << strategy.get(OP_MUL) << "\n";
    ss << "local OP_DIV = " << strategy.get(OP_DIV) << "\n";
    ss << "local OP_IDIV = " << strategy.get(OP_IDIV) << "\n";
    ss << "local OP_MOD = " << strategy.get(OP_MOD) << "\n";
    ss << "local OP_CONCAT = " << strategy.get(OP_CONCAT) << "\n";
    ss << "local OP_LEN = " << strategy.get(OP_LEN) << "\n";
    ss << "local OP_NOT = " << strategy.get(OP_NOT) << "\n";
    ss << "local OP_EQ = " << strategy.get(OP_EQ) << "\n";
    ss << "local OP_LT = " << strategy.get(OP_LT) << "\n";
    ss << "local OP_LE = " << strategy.get(OP_LE) << "\n";
    ss << "local OP_JMP = " << strategy.get(OP_JMP) << "\n";
    ss << "local OP_JMP_FALSE = " << strategy.get(OP_JMP_FALSE) << "\n";
    ss << "local OP_GETGLOBAL = " << strategy.get(OP_GETGLOBAL) << "\n";
    ss << "local OP_SETGLOBAL = " << strategy.get(OP_SETGLOBAL) << "\n";
    ss << "local OP_NEWTABLE = " << strategy.get(OP_NEWTABLE) << "\n";
    ss << "local OP_GETTABLE = " << strategy.get(OP_GETTABLE) << "\n";
    ss << "local OP_SETTABLE = " << strategy.get(OP_SETTABLE) << "\n";
    ss << "local OP_CALL = " << strategy.get(OP_CALL) << "\n";
    ss << "local OP_CLOSURE = " << strategy.get(OP_CLOSURE) << "\n";
    ss << "local OP_GETUPVAL = " << strategy.get(OP_GETUPVAL) << "\n";
    ss << "local OP_SETUPVAL = " << strategy.get(OP_SETUPVAL) << "\n";
    ss << "local OP_VARARG = " << strategy.get(OP_VARARG) << "\n";
    ss << "local OP_FORPREP = " << strategy.get(OP_FORPREP) << "\n";
    ss << "local OP_FORLOOP = " << strategy.get(OP_FORLOOP) << "\n";
    ss << "local OP_TFORCALL = " << strategy.get(OP_TFORCALL) << "\n";
    ss << "local OP_TFORLOOP = " << strategy.get(OP_TFORLOOP) << "\n";
    ss << "local OP_RETURN = " << strategy.get(OP_RETURN) << "\n\n";

    if (encrypt) {
        ss << R"(
local function decrypt_string(t)
    local s = {}
    for i, b in ipairs(t) do
        s[i] = string.char(b ~ 0xAA)
    end
    return table.concat(s)
end

local function decrypt_instruction(t, pc)
    local key = 0xDEADBEEF ~ pc
    return { t[1] ~ key, t[2] ~ key, t[3] ~ key, t[4] ~ key }
end
)";
    }

    ss << "local main_proto = ";
    generateProto(proto, ss, 0, strategy, encrypt);
    ss << "\n";

    // 4. VM Logic - Part 1
    ss << R"(
local _G = _G -- Global environment
local unpack = table.unpack or unpack

-- Forward declaration of run_vm
local run_vm

-- Helper to wrap a closure table into a native Lua function
local function wrap_if_needed(val)
    if type(val) == "table" and val.type == "closure" then
        if not val.wrapper then
            val.wrapper = function(...)
                local args = {...}
                local proto = val.proto
                local numParams = proto.numParams or 0
                local fArgs = {}
                local vArgs = {}
                local n = select('#', ...)

                for i = 1, n do
                    if i <= numParams then
                        fArgs[i] = args[i]
                    else
                        table.insert(vArgs, args[i])
                    end
                end
                vArgs.n = n - numParams
                if vArgs.n < 0 then vArgs.n = 0 end

                return run_vm(val, fArgs, vArgs)
            end
        end
        return val.wrapper
    end
    return val
end

-- Metatable for closures to support pcall/xpcall(arg1)
local closure_mt = {
    __call = function(t, ...)
        return wrap_if_needed(t)(...)
    end
}

run_vm = function(closure, args, varargs)
    local proto = closure.proto
    local stack = {}

    -- Open upvalues: map from stack index to upvalue box
    local open_upvalues = {}

    -- Initialize args
    if args then
        local numParams = proto.numParams or 0
        for i = 1, numParams do
            stack[i-1] = args[i]
        end
    end

    local vargs = varargs or {}

    local pc = 1
    local code = proto.code
    local constants = proto.constants
    local protos = proto.protos
    local upvalues = closure.upvalues or {}

    while pc <= #code do
)";

    // VM Logic - Fetch Instruction
    if (encrypt) {
        ss << "        local inst = decrypt_instruction(code[pc], pc)\n";
    } else {
        ss << "        local inst = code[pc]\n";
    }

    // VM Logic - Part 2
    ss << R"(        pc = pc + 1

        local op = inst[1]
        local a = inst[2]
        local b = inst[3]
        local c = inst[4]

        if op == OP_MOVE then
            stack[a] = stack[b]
            -- If 'a' has an open upvalue, update it
            if open_upvalues[a] then open_upvalues[a].val = stack[a] end
        elseif op == OP_LOADK then
            stack[a] = constants[b]
            if open_upvalues[a] then open_upvalues[a].val = stack[a] end
        elseif op == OP_ADD then
            stack[a] = stack[b] + stack[c]
            if open_upvalues[a] then open_upvalues[a].val = stack[a] end
        elseif op == OP_SUB then
            stack[a] = stack[b] - stack[c]
            if open_upvalues[a] then open_upvalues[a].val = stack[a] end
        elseif op == OP_MUL then
            stack[a] = stack[b] * stack[c]
            if open_upvalues[a] then open_upvalues[a].val = stack[a] end
        elseif op == OP_DIV then
            stack[a] = stack[b] / stack[c]
            if open_upvalues[a] then open_upvalues[a].val = stack[a] end
        elseif op == OP_IDIV then
            stack[a] = math.floor(stack[b] / stack[c])
            if open_upvalues[a] then open_upvalues[a].val = stack[a] end
        elseif op == OP_MOD then
            stack[a] = stack[b] % stack[c]
            if open_upvalues[a] then open_upvalues[a].val = stack[a] end
        elseif op == OP_CONCAT then
            stack[a] = stack[b] .. stack[c]
            if open_upvalues[a] then open_upvalues[a].val = stack[a] end
        elseif op == OP_LEN then
            stack[a] = #stack[b]
            if open_upvalues[a] then open_upvalues[a].val = stack[a] end
        elseif op == OP_NOT then
            stack[a] = not stack[b]
            if open_upvalues[a] then open_upvalues[a].val = stack[a] end
        elseif op == OP_EQ then
            stack[a] = (stack[b] == stack[c])
            if open_upvalues[a] then open_upvalues[a].val = stack[a] end
        elseif op == OP_LT then
            stack[a] = (stack[b] < stack[c])
            if open_upvalues[a] then open_upvalues[a].val = stack[a] end
        elseif op == OP_LE then
            stack[a] = (stack[b] <= stack[c])
            if open_upvalues[a] then open_upvalues[a].val = stack[a] end
        elseif op == OP_JMP then
            pc = pc + b
        elseif op == OP_JMP_FALSE then
            if not stack[a] then
                pc = pc + b
            end
        elseif op == OP_GETGLOBAL then
            stack[a] = _G[constants[b]]
            if open_upvalues[a] then open_upvalues[a].val = stack[a] end
        elseif op == OP_SETGLOBAL then
            _G[constants[b]] = stack[a]
        elseif op == OP_NEWTABLE then
            stack[a] = {}
            if open_upvalues[a] then open_upvalues[a].val = stack[a] end
        elseif op == OP_GETTABLE then
            if stack[b] == nil then
                error("OP_GETTABLE: stack[" .. b .. "] is nil. Key: " .. tostring(stack[c]))
            end
            stack[a] = stack[b][stack[c]]
            if open_upvalues[a] then open_upvalues[a].val = stack[a] end
        elseif op == OP_SETTABLE then
            if stack[a] == nil then
                error("OP_SETTABLE: stack[" .. a .. "] is nil. Key: " .. tostring(stack[b]))
            end
            stack[a][stack[b]] = stack[c]
        elseif op == OP_GETUPVAL then
            stack[a] = upvalues[b].val
            if open_upvalues[a] then open_upvalues[a].val = stack[a] end
        elseif op == OP_SETUPVAL then
            upvalues[b].val = stack[a]
        elseif op == OP_VARARG then
            -- R(A) ... R(A+C-2) = varargs
            local n = c - 1
            if n < 0 then n = vargs.n or #vargs end -- All varargs? B=0.
            -- Using C to determine how many
            -- If C=2, we want 1.
            for i = 1, n do
                stack[a + i - 1] = vargs[i]
            end
            if open_upvalues[a] then open_upvalues[a].val = stack[a] end
        elseif op == OP_FORPREP then
            stack[a] = stack[a] - stack[a+2]
            pc = pc + b
        elseif op == OP_FORLOOP then
            local step = stack[a+2]
            stack[a] = stack[a] + step
            local idx = stack[a]
            local limit = stack[a+1]
            if (step > 0 and idx <= limit) or (step <= 0 and idx >= limit) then
                pc = pc + b
                stack[a+3] = idx
            end
        elseif op == OP_TFORCALL then
            local func = stack[a]
            local state = stack[a+1]
            local ctl = stack[a+2]
            local n = c
            local results
            if type(func) == "function" then
                 results = { func(state, ctl) }
            elseif type(func) == "table" and func.type == "closure" then
                 results = run_vm(func, {state, ctl})
                 -- If results is not a table, wrap it?
                 if type(results) ~= "table" then results = { results } end
            else
                 error("Attempt to call non-function in TFORCALL")
            end

            for i = 1, n do
                stack[a+2+i] = results[i]
            end
        elseif op == OP_TFORLOOP then
            local val = stack[a+1]
            if val ~= nil then
                stack[a] = val
                pc = pc + b
            end
        elseif op == OP_CLOSURE then
            local p = protos[b]
            -- Check if upvalues exist (might be nil if no upvalues)
            local nups = 0
            if p.upvalues then
                 while p.upvalues[nups] do nups = nups + 1 end
            end

            local new_ups = {}
            if p.upvalues then
                for i=0, nups-1 do
                    local info = p.upvalues[i]
                    if info.isLocal then
                        local idx = info.index
                        if open_upvalues[idx] then
                            new_ups[i] = open_upvalues[idx]
                        else
                            local uv = { val = stack[idx] }
                            open_upvalues[idx] = uv
                            new_ups[i] = uv
                        end
                    else
                        new_ups[i] = upvalues[info.index]
                    end
                end
            end
            stack[a] = { proto = p, upvalues = new_ups, type = "closure" }
            setmetatable(stack[a], closure_mt)
            if open_upvalues[a] then open_upvalues[a].val = stack[a] end

        elseif op == OP_CALL then
            local func = stack[a]
            local callArgs = {}
            local numArgs = b - 1
            for i = 1, numArgs do
                callArgs[i] = stack[a + i]
            end

            if type(func) == "table" and func.type == "closure" then
                -- Need to separate fixed args and varargs?
                -- SimpleLua doesn't distinguish well in call site unless we change OP_CALL.
                -- For now, all extra args are just args.
                -- But VM needs to split them for params vs varargs.
                local fixedParams = func.proto.numParams or 0
                local fArgs = {}
                local vArgs = {}
                local fCount = 0
                local vCount = 0
                for i = 1, numArgs do
                    if i <= fixedParams then
                        fCount = fCount + 1
                        fArgs[fCount] = callArgs[i]
                    else
                        vCount = vCount + 1
                        vArgs[vCount] = callArgs[i]
                    end
                end
                vArgs.n = vCount

                local results = { run_vm(func, fArgs, vArgs) }
                local numResults = c - 1
                if numResults > 0 then
                   for i = 1, numResults do
                       stack[a + i - 1] = results[i]
                   end
                   if open_upvalues[a] then open_upvalues[a].val = stack[a] end
                end
            elseif type(func) == "function" then
                 -- Handle native functions that require function arguments (not callable tables)
                 if func == table.sort then
                     if numArgs >= 2 then callArgs[2] = wrap_if_needed(callArgs[2]) end
                 elseif func == xpcall then
                     if numArgs >= 2 then callArgs[2] = wrap_if_needed(callArgs[2]) end
                 elseif func == string.gsub then
                     if numArgs >= 3 then callArgs[3] = wrap_if_needed(callArgs[3]) end
                 end

                 local results = { func(unpack(callArgs, 1, numArgs)) }
                 local numResults = c - 1
                 if numResults > 0 then
                     for i = 1, numResults do
                         stack[a + i - 1] = results[i]
                     end
                     if open_upvalues[a] then open_upvalues[a].val = stack[a] end
                 elseif numResults < 0 then
                     for i = 1, #results do
                         stack[a + i - 1] = results[i]
                     end
                     if open_upvalues[a] then open_upvalues[a].val = stack[a] end
                 end
            else
                -- Try __call metamethod
                local mt = getmetatable(func)
                if mt and mt.__call then
                    -- call mt.__call(func, ...)
                    local metaArgs = {func}
                    for i = 1, numArgs do
                        metaArgs[i + 1] = callArgs[i]
                    end
                    local res = mt.__call(unpack(metaArgs, 1, numArgs + 1))
                     if c - 1 > 0 then
                         stack[a] = res
                         if open_upvalues[a] then open_upvalues[a].val = stack[a] end
                     end
                else
                    error("Attempt to call non-function")
                end
            end

        elseif op == OP_RETURN then
            local n = b - 1
            if n <= 0 then
                return
            elseif n == 1 then
                return stack[a]
            else
                local res = {}
                for i = 0, n - 1 do
                    res[i + 1] = stack[a + i]
                end
                return unpack(res)
            end
        else
            error("Unknown opcode: " .. op)
        end
    end
end

-- Run main chunk
run_vm({ proto = main_proto, upvalues = {} }, {})
)";

    std::string result = ss.str();
    if (pack) {
        result = minify(result);
    }
    out << result;
}

void LuaGenerator::generateProto(Prototype* proto, std::ostream& out, int index, const OpCodeStrategy& strategy, bool encrypt) {
    (void)index;
    out << "{\n";

    out << "  numParams = " << proto->numParams << ",\n";

    // Constants
    out << "  constants = {\n";
    for (size_t i = 0; i < proto->constants.size(); ++i) {
        const Value& v = proto->constants[i];
        out << "    [" << i << "] = ";
        if (is_number(v)) {
            out << as_number(v);
        } else if (is_boolean(v)) {
            out << (as_boolean(v) ? "true" : "false");
        } else if (is_string(v)) {
            if (encrypt) {
                out << encryptString(as_string(v));
            } else {
                out << "\"" << as_string(v) << "\"";
            }
        } else {
            out << "nil";
        }
        out << ",\n";
    }
    out << "  },\n";

    // Instructions
    out << "  code = {\n";
    for (size_t i = 0; i < proto->instructions.size(); ++i) {
        const Instruction& inst = proto->instructions[i];
        if (encrypt) {
             out << "    " << encryptInstruction(strategy.get(inst.op), inst.a, inst.b, inst.c, i + 1) << ",\n";
        } else {
             out << "    {" << strategy.get(inst.op) << ", " << inst.a << ", " << inst.b << ", " << inst.c << "},\n";
        }
    }
    out << "  },\n";

    // Nested Prototypes (References)
    out << "  protos = {\n";
    for (size_t i = 0; i < proto->protos.size(); ++i) {
         out << "    [" << i << "] = ";
         generateProto(proto->protos[i], out, i, strategy, encrypt);
         out << ",\n";
    }
    out << "  },\n";

    // Upvalues metadata
    out << "  upvalues = {\n";
    for (size_t i = 0; i < proto->upvalues.size(); ++i) {
        out << "    [" << i << "] = { isLocal = " << (proto->upvalues[i].isLocal ? "true" : "false")
            << ", index = " << proto->upvalues[i].index << " },\n";
    }
    out << "  }\n";

    out << "}";
}

static std::string encryptString(const std::string& s) {
    std::stringstream ss;
    ss << "decrypt_string({";
    for (size_t i = 0; i < s.length(); ++i) {
        ss << (int)((unsigned char)s[i] ^ 0xAA);
        if (i < s.length() - 1) ss << ",";
    }
    ss << "})";
    return ss.str();
}

static std::string encryptInstruction(int op, int a, int b, int c, int pc) {
    unsigned int key = 0xDEADBEEF ^ pc;
    std::stringstream ss;
    ss << "{" << (op ^ key) << ", " << (a ^ key) << ", " << (b ^ key) << ", " << (c ^ key) << "}";
    return ss.str();
}

static std::string minify(std::string code) {
    std::string res;
    bool inString = false;
    bool inComment = false;
    for (size_t i = 0; i < code.length(); ++i) {
        char c = code[i];
        if (inComment) {
            if (c == '\n') {
                inComment = false;
                res += ' ';
            }
        } else if (inString) {
            res += c;
            if (c == '"') {
                // Check if the quote is escaped by counting preceding backslashes
                int backslashes = 0;
                size_t j = i;
                while (j > 0 && code[j-1] == '\\') {
                    backslashes++;
                    j--;
                }
                if (backslashes % 2 == 0) {
                    inString = false;
                }
            }
        } else {
            if (c == '-' && i + 1 < code.length() && code[i+1] == '-') {
                inComment = true;
                i++; // Skip second '-'
            } else if (c == '"') {
                inString = true;
                res += c;
            } else if (isspace(c)) {
                if (res.empty() || res.back() != ' ') res += ' ';
            } else {
                res += c;
            }
        }
    }
    return res;
}
