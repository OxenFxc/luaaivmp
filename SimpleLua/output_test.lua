local OP_MOVE = 0
local OP_LOADK = 1
local OP_ADD = 2
local OP_SUB = 3
local OP_MUL = 4
local OP_DIV = 5
local OP_MOD = 6
local OP_CONCAT = 7
local OP_LEN = 8
local OP_NOT = 9
local OP_EQ = 10
local OP_LT = 11
local OP_LE = 12
local OP_JMP = 13
local OP_JMP_FALSE = 14
local OP_GETGLOBAL = 15
local OP_SETGLOBAL = 16
local OP_NEWTABLE = 17
local OP_GETTABLE = 18
local OP_SETTABLE = 19
local OP_CALL = 20
local OP_CLOSURE = 21
local OP_GETUPVAL = 22
local OP_SETUPVAL = 23
local OP_VARARG = 24
local OP_FORPREP = 25
local OP_FORLOOP = 26
local OP_PRINT = 27
local OP_RETURN = 28

local main_proto = {
  constants = {
    [0] = 1,
    [1] = 2,
    [2] = "table",
    [3] = "insert",
    [4] = 3,
    [5] = 1,
    [6] = 1,
    [7] = 1,
    [8] = 10,
  },
  code = {
    {21, 1, 0, 0},
    {0, 0, 1, 0},
    {17, 1, 0, 0},
    {0, 2, 1, 0},
    {1, 3, 0, 0},
    {1, 4, 1, 0},
    {19, 2, 3, 4},
    {15, 3, 2, 0},
    {1, 4, 3, 0},
    {18, 5, 3, 4},
    {1, 6, 4, 0},
    {0, 6, 2, 0},
    {0, 7, 6, 0},
    {20, 5, 3, 1},
    {17, 3, 0, 0},
    {0, 4, 3, 0},
    {1, 5, 5, 0},
    {17, 6, 0, 0},
    {19, 4, 5, 6},
    {1, 5, 6, 0},
    {18, 6, 4, 5},
    {1, 7, 7, 0},
    {1, 8, 8, 0},
    {19, 6, 7, 8},
    {28, 0, 0, 0},
  },
  protos = {
    [0] = {
  constants = {
  },
  code = {
    {28, 0, 1, 0},
  },
  protos = {
  },
  upvalues = {
  }
},
  },
  upvalues = {
  }
}

local _G = _G -- Global environment

local function run_vm(closure, args, varargs)
    local proto = closure.proto
    local stack = {}

    -- Open upvalues: map from stack index to upvalue box
    local open_upvalues = {}

    -- Initialize args
    if args then
        for i = 1, #args do
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
            stack[a] = stack[b][stack[c]]
            if open_upvalues[a] then open_upvalues[a].val = stack[a] end
        elseif op == OP_SETTABLE then
            stack[a][stack[b]] = stack[c]
        elseif op == OP_GETUPVAL then
            stack[a] = upvalues[b].val
            if open_upvalues[a] then open_upvalues[a].val = stack[a] end
        elseif op == OP_SETUPVAL then
            upvalues[b].val = stack[a]
        elseif op == OP_VARARG then
            -- R(A) ... R(A+C-2) = varargs
            local n = c - 1
            if n < 0 then n = #vargs end -- All varargs? B=0.
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
        elseif op == OP_CLOSURE then
            local p = protos[b]
            -- Check if upvalues exist (might be nil if no upvalues)
            local nups = 0
            if p.upvalues then nups = #p.upvalues + 1 end

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
                for i = 1, #callArgs do
                    if i <= fixedParams then
                        table.insert(fArgs, callArgs[i])
                    else
                        table.insert(vArgs, callArgs[i])
                    end
                end

                local results = run_vm(func, fArgs, vArgs)
                local numResults = c - 1
                if numResults > 0 then
                   stack[a] = results
                   if open_upvalues[a] then open_upvalues[a].val = stack[a] end
                end
            elseif type(func) == "function" then
                 local res = func(table.unpack(callArgs))
                 if c - 1 > 0 then
                     stack[a] = res
                     if open_upvalues[a] then open_upvalues[a].val = stack[a] end
                 end
            else
                -- Try __call metamethod
                local mt = getmetatable(func)
                if mt and mt.__call then
                    -- call mt.__call(func, ...)
                    local metaArgs = {func}
                    for i, arg in ipairs(callArgs) do
                        table.insert(metaArgs, arg)
                    end
                    local res = mt.__call(table.unpack(metaArgs))
                     if c - 1 > 0 then
                         stack[a] = res
                         if open_upvalues[a] then open_upvalues[a].val = stack[a] end
                     end
                else
                    error("Attempt to call non-function")
                end
            end

        elseif op == OP_PRINT then
            print(stack[a])
        elseif op == OP_RETURN then
            -- Close upvalues?
            -- Since we used tables {val=...} for upvalues, and passed them by reference to children,
            -- we don't need to do explicit closing logic unless we want to detach them from stack.
            -- But stack is gone anyway.
            if b == 2 then
                return stack[a]
            end
            return
        else
            error("Unknown opcode: " .. op)
        end
    end
end

-- Run main chunk
run_vm({ proto = main_proto, upvalues = {} }, {})
