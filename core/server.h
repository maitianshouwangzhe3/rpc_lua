#pragma once

#include "luna.h"
#include "connect.h"

#include <memory>
#include <vector>

class poller;
struct lua_State;
struct callback_context {
    lua_State* L;
};

class server final {
public:
    server();
    ~server();

    bool listen(std::string ip, int port);

    void step(int timeout);

    void start(std::string ip, int port);

    void add_connect(std::shared_ptr<connect> conn);

    void call_back(callback_context* ctx);

    void dispatch(connect* conn);

    int push(int fd, const char* data, int len);

    std::shared_ptr<poller> get_poller();
private:
    int init(std::string& ip, int port);
public:
    DECLARE_LUA_CLASS(server)
private:
    bool is_stop_;
    callback_context* callback_ctx_;
    std::shared_ptr<poller> poller_;
    std::vector<std::shared_ptr<connect>> connect_pool_;
};