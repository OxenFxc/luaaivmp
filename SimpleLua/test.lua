local function f() end

local t = {}
t[1] = 2

table.insert(t, 3)

local stack = {}
stack[1] = {}
stack[1][1] = 10
