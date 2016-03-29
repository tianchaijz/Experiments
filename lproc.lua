-- load our concurrent programming library
require "luaproc"

-- create a new lua process
luaproc.start([=[
    luaproc.send("channel", "Hello")
    print(luaproc.receive("channel"))
]=])

-- create a new lua process
luaproc.start([=[
    print(luaproc.receive("channel"))
    luaproc.send("channel", "World")
]=])

-- wait until all lua processes
-- have finished before exiting
luaproc.exit()
