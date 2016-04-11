local socket = require "socket"


local download, receive, get, dispatch
local threads, host, port


function download(host, file)
    local conn = socket.tcp()
    conn:settimeout(5, "block")
    local ok, err = conn:connect(host, port)
    if not ok then
        print("error:", err)
        return
    end

    -- counts number of bytes read
    local count = 0
    conn:send("GET " .. file .. " HTTP/1.0\r\n\r\n")
    while true do
        local s, status = receive(conn)
        count = count + #s

        if status == "closed" then break end
    end

    conn:close()
    print(file, count)
end


function receive(conn)
    -- do not block
    conn:settimeout(0)
    local s, status, partial = conn:receive(2^10)
    if status == "timeout" then
        coroutine.yield(conn)
    end

    return s or partial, status
end


function get(host, file)
    -- create coroutine
    local co = coroutine.create(function()
        download(host, file)
    end)
    -- insert it in the list
    table.insert(threads, co)
end


function dispatch()
    local i = 1
    while true do
        -- no more threads?
        if threads[i] == nil then
            -- list is empty?
            if threads[1] == nil then break end
            -- restart the loop
            i = 1
        end

        local status, res = coroutine.resume(threads[i])
        -- thread finished its task?
        if status == false or not res then
            table.remove(threads, i)
        else
            -- go to next thread
            i = i + 1
        end
    end
end


threads, host, port = {}, "127.0.0.1", 3101

get(host, "/0")
get(host, "/1")
get(host, "/2")
get(host, "/3")

-- simulate connection timeout
-- iptables -I INPUT -s 127.0.0.1 -p tcp --dport 3101 -j DROP

-- main loop
dispatch()
