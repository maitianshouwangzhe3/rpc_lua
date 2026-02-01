
#include "socket.h"
#include "client.h"
#include "connect_server.h"

LUA_EXPORT_CLASS_BEGIN(client)
LUA_EXPORT_METHOD(invoke)
LUA_EXPORT_METHOD(init)
LUA_EXPORT_METHOD(send_data)
LUA_EXPORT_CLASS_END()

client::client(): connect_(nullptr) {

}

client::~client() {

}

std::string client::invoke(const char* request, int len) {
    connect_->write(const_cast<char*>(request), len);
    int nready = connect_->read();
    std::string ret = "";
    ret.reserve(nready);
    connect_->get_socket()->put(ret.data(), nready);
    return ret;
}

int client::init(std::string ip, int port) {
    std::shared_ptr<socket> sock = std::make_shared<socket>();
    
    int ret = sock->connect(ip.data(), port);
    if (ret != 0) {
        return ret;
    }

    connect_ = std::make_shared<connect_server>(sock);
    return ret;
}

int client::send_data(const char* data, int len) {
    return connect_->write(const_cast<char*>(data), len);
}