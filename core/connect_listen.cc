
#include "server.h"
#include "poller.h"
#include "connect_listen.h"
#include "connect_client.h"

#include <memory>
connect_listen::connect_listen(protocol protocol): svr_(nullptr) {
    socket_ = std::make_shared<rpc_lua::socket>(protocol);
}

connect_listen::~connect_listen() {

}

int connect_listen::dispatch(net_event& event) {
    if (!svr_) {
        return -1;
    }

    if (event.event & static_cast<uint32_t>(net_event_type::EVENT_READ)) {
        int fd = on_accept_message();
        std::shared_ptr<connect> conn = std::make_shared<connect_client>(fd);
        conn->set_server(svr_);
        svr_->add_connect(conn);
        conn->get_socket()->set_fd_non_block();
        auto poller_imp = svr_->get_poller();
        poller_imp->add_event(fd, static_cast<int>(net_event_type::EVENT_READ), false, socket_.get());
    }

    return -1;
}

int connect_listen::on_accept_message() {
    return socket_->accept();
}

std::shared_ptr<rpc_lua::socket> connect_listen::get_socket() {
    return socket_;
}

int connect_listen::close() {
    int fd = socket_->get_fd();
    return 0;
}

void connect_listen::set_server(server* svr) {
    svr_ = svr;
}

int connect_listen::write(char* data, int len) {
    return 0;
}

int connect_listen::read() {
    return 0;
}