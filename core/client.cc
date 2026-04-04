
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
    std::string ret = "";
    connect_->write(const_cast<char*>(request), len);
    int nready = connect_->read();
    if (nready <= 0) {
        return ret;
    }

    uint32_t length = 0;
    connect_->get_socket()->put((char*)&length, sizeof(length));
    if (length <= 0) {
        return ret;
    }
    ret.reserve(length);
    connect_->get_socket()->put(ret.data(), length);
    return ret;
}

int client::init(std::string ip, int port) {
    std::shared_ptr<rpc_lua::socket> sock = std::make_shared<rpc_lua::socket>();
    
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