

package.cpath = package.cpath .. ";./build/?.so"
package.path = package.path .. ";./lualib/?.lua;./bin/lua/?.lua"

local rpc_manager = require "rpc_manager"
local rpc_pb = require "rpc.pb"
rpc_manager.import("/home/ouyangjun/code/github/rpc_lua/bin")

local service = rpc_manager.rpc_service_info["say_hello_function"]
if not service then
    return nil
end
print(service.service_name)
local data = rpc_pb.encode(service.input_type, { name = "lua" })
local req = {
                service_name = service.service_name,
                func_name = "say_hello",
                args = data
            }
local req_data = rpc_pb.encode(".rpc.rpc_requst", req)

local rpc_req = rpc_pb.decode(".rpc.rpc_requst", req_data)
if not rpc_req then
    print("rpc_req is nil")
    return
end
for key, value in pairs(rpc_req) do
    print("key:", key, "value:", value)
end
print("pc_req.func_name:", rpc_req.func_name)
local service = rpc_manager.rpc_service_info[rpc_req.service_name .. "_service"]
if not service then
    print("service is nil")
    return
end

local rpc_req_data = rpc_pb.decode(service.input_type, rpc_req.args)
if not rpc_req_data then
    print("rpc_req_data is nil")
    return
end

print("name:", rpc_req_data.name)

local tmp1 = {message = "hello " .. rpc_req_data.name}
local rpc_ret_data = rpc_pb.encode(service.output_type, tmp1)

local ret1 = {code = 0, result = rpc_ret_data}
local ret2 = rpc_pb.encode(".rpc.rpc_response", ret1)
print("len:", #ret2)
local ret3 = rpc_pb.decode(".rpc.rpc_response", ret2)

print("code:", ret3.code)

local ret4 = rpc_pb.decode(service.output_type, ret3.result)

print("message:", #ret4.message)