local lcounter = require "lcounter"

local c = lcounter.new(0, "c1")

c:add(2)
c:decrement()

print("val= " .. c:getval())

c:subtract(-2)
c:increment()

print("tostring: " .. tostring(c))
