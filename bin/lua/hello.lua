
function _G.c2s.hello_service.say_hello(req_body)
    print("hello_service.say_hello", req_body.name)
    return { message = "hello " .. req_body.name }
end