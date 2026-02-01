package.cpath = package.cpath .. ";./build/?.so"
package.path = package.path .. ";./lualib/?.lua;./bin/lua/?.lua"

local rpc_manager = require "rpc_manager"
rpc_manager.import("/home/ouyangjun/code/github/rpc_lua/bin")
local ok = rpc_manager.client_init("127.0.0.1", 8082)
if ok == 0 then
    print("connect ok")
    local ret = rpc_manager.say_hello({name = "lua"})
    print("-------------")
    print("ret:", ret.message)
else
    print("connect error", ok)
end
