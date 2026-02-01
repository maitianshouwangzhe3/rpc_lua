
package.cpath = package.cpath .. ";./build/?.so"
package.path = package.path .. ";./lualib/?.lua;./bin/lua/?.lua"

-- local rpc = require "rpc"
-- local fs = require "rpc.lfs"

-- _G.rpc_callback = function()
--   print("rpc callback")
-- end
-- _G.rpc_server = rpc.new_server()
-- rpc.callback(_G.rpc_callback)

-- _G.rpc_client = rpc.new_client()
-- print(_G.rpc_client)
-- _G.rpc_server.start()

local rpc_manager = require "rpc_manager"
rpc_manager.import("/home/ouyangjun/code/github/rpc_lua/bin")
require "hello"
rpc_manager.server_start("127.0.0.1", 8082)

