local OP_MOVE = 0
local OP_LOADK = 1
local OP_ADD = 2
local OP_SUB = 3
local OP_MUL = 4
local OP_DIV = 5
local OP_IDIV = 6
local OP_MOD = 7
local OP_CONCAT = 8
local OP_LEN = 9
local OP_NOT = 10
local OP_EQ = 11
local OP_LT = 12
local OP_LE = 13
local OP_JMP = 14
local OP_JMP_FALSE = 15
local OP_GETGLOBAL = 16
local OP_SETGLOBAL = 17
local OP_NEWTABLE = 18
local OP_GETTABLE = 19
local OP_SETTABLE = 20
local OP_CALL = 21
local OP_CLOSURE = 22
local OP_GETUPVAL = 23
local OP_SETUPVAL = 24
local OP_VARARG = 25
local OP_FORPREP = 26
local OP_FORLOOP = 27
local OP_TFORCALL = 28
local OP_TFORLOOP = 29
local OP_RETURN = 30

local main_proto = {
  numParams = 0,
  constants = {
    [0] = "print",
    [1] = 5,
  },
  code = {
    {22, 1, 0, 0},
    {0, 0, 1, 0},
    {16, 1, 0, 0},
    {0, 2, 1, 0},
    {0, 3, 0, 0},
    {1, 4, 1, 0},
    {0, 4, 4, 0},
    {21, 3, 2, 2},
    {0, 3, 3, 0},
    {21, 2, 2, 1},
    {30, 0, 0, 0},
  },
  protos = {
    [0] = {
  numParams = 1,
  constants = {
    [0] = 0,
    [1] = 1,
    [2] = 1,
  },
  code = {
    {1, 1, 0, 0},
    {11, 2, 0, 1},
    {15, 2, 4, 0},
    {1, 3, 1, 0},
    {0, 4, 3, 0},
    {30, 4, 2, 0},
    {14, 0, 9, 0},
    {23, 1, 0, 0},
    {0, 2, 1, 0},
    {1, 3, 2, 0},
    {3, 4, 0, 3},
    {0, 3, 4, 0},
    {21, 2, 2, 2},
    {4, 5, 0, 2},
    {0, 6, 5, 0},
    {30, 6, 2, 0},
    {30, 0, 1, 0},
  },
  protos = {
  },
  upvalues = {
    [0] = { isLocal = true, index = 0 },
  }
},
  },
  upvalues = {
  }
}

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
