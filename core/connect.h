#pragma once

#include <memory>

namespace rpc_lua {
    class socket;
}

class server;
struct net_event;

class connect {
public:
    virtual int dispatch(net_event& event) = 0;
    virtual std::shared_ptr<rpc_lua::socket> get_socket() = 0;
    virtual int close() = 0;
    virtual void set_server(server* svr) = 0;
    virtual int write(char* data, int len) = 0;
    virtual int read() = 0;
};