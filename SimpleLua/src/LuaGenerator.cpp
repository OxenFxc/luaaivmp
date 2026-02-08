#include "LuaGenerator.h"
#include <vector>

void LuaGenerator::generate(Prototype* proto, std::ostream& out) {
    // 1. Opcodes definitions
    out << "local OP_MOVE = " << OP_MOVE << "\n";
    out << "local OP_LOADK = " << OP_LOADK << "\n";
    out << "local OP_ADD = " << OP_ADD << "\n";
    out << "local OP_SUB = " << OP_SUB << "\n";
    out << "local OP_MUL = " << OP_MUL << "\n";
    out << "local OP_DIV = " << OP_DIV << "\n";
    out << "local OP_IDIV = " << OP_IDIV << "\n";
    out << "local OP_MOD = " << OP_MOD << "\n";
    out << "local OP_CONCAT = " << OP_CONCAT << "\n";
    out << "local OP_LEN = " << OP_LEN << "\n";
    out << "local OP_NOT = " << OP_NOT << "\n";
    out << "local OP_EQ = " << OP_EQ << "\n";
    out << "local OP_LT = " << OP_LT << "\n";
    out << "local OP_LE = " << OP_LE << "\n";
    out << "local OP_JMP = " << OP_JMP << "\n";
    out << "local OP_JMP_FALSE = " << OP_JMP_FALSE << "\n";
    out << "local OP_GETGLOBAL = " << OP_GETGLOBAL << "\n";
    out << "local OP_SETGLOBAL = " << OP_SETGLOBAL << "\n";
    out << "local OP_NEWTABLE = " << OP_NEWTABLE << "\n";
    out << "local OP_GETTABLE = " << OP_GETTABLE << "\n";
    out << "local OP_SETTABLE = " << OP_SETTABLE << "\n";
    out << "local OP_CALL = " << OP_CALL << "\n";
    out << "local OP_CLOSURE = " << OP_CLOSURE << "\n";
    out << "local OP_GETUPVAL = " << OP_GETUPVAL << "\n";
    out << "local OP_SETUPVAL = " << OP_SETUPVAL << "\n";
    out << "local OP_VARARG = " << OP_VARARG << "\n";
    out << "local OP_FORPREP = " << OP_FORPREP << "\n";
    out << "local OP_FORLOOP = " << OP_FORLOOP << "\n";
    out << "local OP_TFORCALL = " << OP_TFORCALL << "\n";
    out << "local OP_TFORLOOP = " << OP_TFORLOOP << "\n";
    out << "local OP_RETURN = " << OP_RETURN << "\n\n";

    out << "local main_proto = ";
    generateProto(proto, out, 0);
    out << "\n";

    // 4. VM Logic
    out << R"(
local _G = _G -- Global environment
local unpack = table.unpack or unpack

local function run_vm(closure, args, varargs)
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
        local inst = code[pc]
        pc = pc + 1

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
}

void LuaGenerator::generateProto(Prototype* proto, std::ostream& out, int index) {
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
            out << "\"" << as_string(v) << "\"";
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
        out << "    {" << inst.op << ", " << inst.a << ", " << inst.b << ", " << inst.c << "},\n";
    }
    out << "  },\n";

    // Nested Prototypes (References)
    out << "  protos = {\n";
    for (size_t i = 0; i < proto->protos.size(); ++i) {
         out << "    [" << i << "] = ";
         generateProto(proto->protos[i], out, i);
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
