#pragma once

#include "poller.h"
#include "socket.h"
#include "connect.h"

class connect_client final: public connect { 
public:
    connect_client();
    connect_client(int fd);
    connect_client(std::shared_ptr<socket> sock);
    ~connect_client();

    virtual int dispatch(net_event& event) override;

    char* recv_data();

    int send_data(char* data, int len);

    virtual std::shared_ptr<socket> get_socket() override;

    virtual int close() override;

    virtual void set_server(server* svr) override;

    virtual int write(char* data, int len) override;

    virtual int read() override;
private:
    int event_read_hander();

    int event_write_hander();

    int event_rdhup_hander();

    int event_close_hander();

private:
    server* svr_;
    std::shared_ptr<socket> socket_;
};