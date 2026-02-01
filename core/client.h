#pragma once

#include "luna.h"
#include "connect.h"

#include <memory>
#include <string>

class client final {
public:
    client();
    ~client();

    std::string invoke(const char* request, int len);
    int init(std::string ip, int port);
    int send_data(const char* data, int len);
public:
    DECLARE_LUA_CLASS(client)
private:
    std::shared_ptr<connect> connect_;
};