return function()
    if not cnt then
        cnt = 0
    end

    for k, v in pairs(ngx.status) do
        cnt = cnt + 1
        print("*** ", cnt, ": ", k, " => ", v)
    end

    while true do
        print("*** max_cycle = ", max_cycle)
        ngx.foo()
        ngx.bar()
    end
end
