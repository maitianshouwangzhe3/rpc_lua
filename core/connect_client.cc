
#include "server.h"
#include "connect_client.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>

connect_client::connect_client(): svr_(nullptr) {

}
connect_client::connect_client(int fd): svr_(nullptr) {
    socket_ = std::make_shared<socket>(fd);
}

connect_client::connect_client(std::shared_ptr<socket> sock): svr_(nullptr) {
    this->socket_ = sock;
}

connect_client::~connect_client() {

}

int connect_client::event_read_hander() {
    int nready = socket_->recv();
    if (nready < 0) {
        return nready;
    } else if (nready == 0) {
        return event_close_hander();
    }

    svr_->dispatch(this);
    return nready;
}

int connect_client::event_write_hander() {
    (void)socket_->send();
    return 0;
}

int connect_client::event_rdhup_hander() {
    return 0;
}

int connect_client::event_close_hander() {
    socket_->close();
    return 0;
}

int connect_client::dispatch(net_event& event) {
    if (event.event & static_cast<uint32_t>(net_event_type::EVENT_RDHUP)) {
        return event_rdhup_hander();
    } else {
        if (event.event & static_cast<uint32_t>(net_event_type::EVENT_READ)) {
            (void)event_read_hander();
        }

        if (event.event & static_cast<uint32_t>(net_event_type::EVENT_WRITE)) {
            (void)event_write_hander();
            svr_->get_poller()->mod_event(event.fd, static_cast<uint32_t>(net_event_type::EVENT_READ));
        }
    }

    return 0;
}

char* connect_client::recv_data() {
    int len = socket_->get_rbuffer()->buffer_len();
    char* data = (char*)malloc(len);
    memset(data, 0, len);
    socket_->get_rbuffer()->buffer_remove(data, len);
    return data;
}

int connect_client::send_data(char* data, int len) {
    return socket_->get_wbuffer()->buffer_add(data, len);
}

std::shared_ptr<socket> connect_client::get_socket() {
    return socket_;
}

int connect_client::close() {
    return event_close_hander();
}

void connect_client::set_server(server* svr) {
    svr_ = svr;
}

int connect_client::write(char* data, int len) {
    return socket_->push(data, len);
}

int connect_client::read() {
    return socket_->recv();
}