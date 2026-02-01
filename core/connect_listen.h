#pragma once

#include "poller.h"
#include "socket.h"
#include "connect.h"

class connect_listen final: public connect {
public:
    connect_listen(protocol protocol = protocol::TCP);

    ~connect_listen();

    virtual int dispatch(net_event& event) override;

    virtual std::shared_ptr<socket> get_socket() override;

    virtual int close() override;

    virtual void set_server(server* svr) override;

    virtual int write(char* data, int len) override;

    virtual int read() override;

private:
    int on_accept_message();

private:
    server* svr_;
    std::shared_ptr<socket> socket_;
};