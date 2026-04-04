#pragma once

#include "poller.h"
#include "socket.h"
#include "connect.h"

class connect_server final: public connect {
public:
    connect_server();
    connect_server(std::shared_ptr<rpc_lua::socket> sock);
    ~connect_server();

    virtual int dispatch(net_event& event) override;

    virtual std::shared_ptr<rpc_lua::socket> get_socket() override;

    virtual int close() override;

    int connect(const char* ip, int port);

    virtual void set_server(server* svr) override;

    virtual int write(char* data, int len) override;

    virtual int read() override;

private:
    int event_read_hander();

    int event_write_hander();

    int event_rdhup_hander();

    int event_close_hander();
private:
    enum class tatus {
        CONNECTING = 0,
        CONNECTED,
        DISCONNECTED,
    };

    tatus statu_;
    server* svr_;
    std::shared_ptr<rpc_lua::socket> socket_;
};