local function printResult(a)
    for i = 1, #a do
        io.write(a[i], " ")
    end
    io.write("\n")
end


local function permgen(a, n)
    -- default for 'n' is size of 'a'
    n = n or #a
    -- nothing to change?
    if n <= 1 then
        coroutine.yield(a)
    else
        for i = 1, n do
            -- put i-th element as the last one
            a[n], a[i] = a[i], a[n]
            -- generate all permutations of the other elements
            permgen(a, n - 1)
            -- restore i-th element
            a[n], a[i] = a[i], a[n]
        end
    end
end


local function permutations(a)
    local co = coroutine.create(function() permgen(a) end)
    -- iterator
    return function()
        local code, res = coroutine.resume(co)
        return res
    end
end


for p in permutations({ "a", "b", "c", "d" }) do
    printResult(p)
end
