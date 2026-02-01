

#include "connect_server.h"

#include <cerrno>
connect_server::connect_server(): svr_(nullptr) {

}

connect_server::connect_server(std::shared_ptr<socket> socket): socket_(socket), svr_(nullptr) {
    // socket_->set_fd_non_block();
}

connect_server::~connect_server() {

}

int connect_server::dispatch(net_event& event) {
    switch (statu_) {
        case tatus::CONNECTING: {
            if (event.event & static_cast<uint32_t>(net_event_type::EVENT_WRITE)) {

            }
        }
        break;
        case tatus::CONNECTED: {
            if (event.event & static_cast<uint32_t>(net_event_type::EVENT_RDHUP)) {
                return event_rdhup_hander();
            } else {
                if (event.event & static_cast<uint32_t>(net_event_type::EVENT_READ)) {
                    (void)event_read_hander();
                }

                if (event.event & static_cast<uint32_t>(net_event_type::EVENT_WRITE)) {
                    (void)event_write_hander();
                }
            }
        }
        break;
        case tatus::ERROR: {

        }
        break;
        default: {

        }
    }
    return 0;
}

std::shared_ptr<socket> connect_server::get_socket() {
    return socket_;
}

int connect_server::close() {
    return event_close_hander();
}

int connect_server::connect(const char* ip, int port) {
    statu_ = tatus::CONNECTING;
    int ret = socket_->connect(ip, port);
    if (ret == -1) {
        if (errno == EINPROGRESS) {
            
        } else {
            statu_ = tatus::ERROR;
            return -1;
        }
    }

    return ret;
}

int connect_server::event_read_hander() {
    int nready = socket_->recv();
    if (nready < 0) {
        return nready;
    } else if (nready == 0) {
        return event_close_hander();
    }

    return nready;
}

int connect_server::event_write_hander() {
    int fd = socket_->get_fd();
    (void)socket_->send();
    return 0;
}

int connect_server::event_rdhup_hander() {
    return 0;
}

int connect_server::event_close_hander() {
    int fd = socket_->get_fd();
    return 0;
}

void connect_server::set_server(server* svr) {
    svr_ = svr;
}

int connect_server::write(char* data, int len) {
    socket_->push(data, len);
    return socket_->send();
}

int connect_server::read() {
    return socket_->read();
}