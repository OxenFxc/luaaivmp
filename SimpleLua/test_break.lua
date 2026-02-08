-- Test While Loop Break
local i = 0
while i < 10 do
    if i == 5 then
        break
    end
    i = i + 1
end
if i ~= 5 then
    print("FAIL: While loop break failed. i=" .. i)
else
    print("PASS: While loop break")
end

-- Test Numeric For Loop Break
local sum = 0
for j = 1, 10 do
    sum = sum + j
    if j == 5 then
        break
    end
end
if sum ~= 15 then -- 1+2+3+4+5 = 15
    print("FAIL: Numeric For loop break failed. sum=" .. sum)
else
    print("PASS: Numeric For loop break")
end

-- Test Generic For Loop Break (using ipairs)
local t = {10, 20, 30, 40, 50}
local found = false
for k, v in ipairs(t) do
    if v == 30 then
        found = true
        break
    end
end
if not found then
    print("FAIL: Generic For loop break failed (not found)")
else
    print("PASS: Generic For loop break")
end

-- Test Nested Loop Break
local outer = 0
local inner = 0
while outer < 5 do
    outer = outer + 1
    local j = 0
    while j < 5 do
        j = j + 1
        if j == 3 then
            break -- Should break inner loop only
        end
    end
    inner = j
end

if outer ~= 5 or inner ~= 3 then
    print("FAIL: Nested loop break failed. outer=" .. outer .. " inner=" .. inner)
else
    print("PASS: Nested loop break")
end

-- Test complex condition break (User reported issue)
local ok = false
local val = 10
local max = 5
local broken = false
while true do
    if not ok or val > max then
        broken = true
        break
    end
    print("Should not be reached")
end
if not broken then
    print("FAIL: Complex condition break failed")
else
    print("PASS: Complex condition break")
end
