
#include "server.h"
#include "poller.h"
#include "connect_listen.h"

#include <iostream>

LUA_EXPORT_CLASS_BEGIN(server)
LUA_EXPORT_METHOD(start)
LUA_EXPORT_METHOD(push)
LUA_EXPORT_METHOD(listen)
LUA_EXPORT_METHOD(step)
LUA_EXPORT_CLASS_END()

server::server(): poller_(std::make_shared<poller>()), is_stop_(false), callback_ctx_(nullptr) {
    connect_pool_.reserve(1024);
}

server::~server() {

}

void server::add_connect(std::shared_ptr<connect> conn) {
    int fd = conn->get_socket()->get_fd();
    
    // 确保容器大小足够
    if (fd >= connect_pool_.capacity()) {
        connect_pool_.reserve(fd + 1);
    }
    
    connect_pool_[fd] = conn;
    // connect_pool_.resize(fd + 1);
}

std::shared_ptr<poller> server::get_poller() {
    return poller_;
}

void server::start(std::string ip, int port) {
    init(ip, port);
    while(!is_stop_) {
        std::vector<net_event> evs;
        int nready = poller_->poll(evs);
        if (nready > 0) {
            for (auto &event: evs) {
                if (event.fd > connect_pool_.capacity()) {
                    continue;
                }

                auto connect = connect_pool_[event.fd];
                if (connect) {
                    connect->dispatch(event);
                }
            }
        } 
    }
}

bool server::listen(std::string ip, int port) {
    return 0 == init(ip, port);
}

void server::step(int timeout) {
    std::vector<net_event> evs;
    int nready = poller_->poll(evs);
    if (nready > 0) {
        for (auto &event: evs) {
            if (event.fd > connect_pool_.capacity()) {
                continue;
            }

            auto connect = connect_pool_[event.fd];
            if (connect) {
                connect->dispatch(event);
            }
        }
    }
}

int server::init(std::string& ip, int port) {
    std::shared_ptr<connect> conn = std::make_shared<connect_listen>();
    if (!conn) {
        return -1;
    }

    auto sk = conn->get_socket();
    if (!sk) {
        return -1;
    }

    sk->set_fd_non_block();
    bool ret = sk->bind(ip, port);
    if (!ret) {
        return -1;
    }

    int listen_ret = sk->listen();
    if (listen_ret != 0) {
        return -1;
    }

    int fd = sk->get_fd();
    conn->set_server(this);
    int poller_ret = poller_->add_event(fd, static_cast<int>(net_event_type::EVENT_READ), false);
    if (poller_ret != 0) {
        return -1;
    }

    add_connect(conn);
    return 0;
}

void server::call_back(callback_context* ctx) {
    callback_ctx_ = ctx;
}

void server::dispatch(connect* conn) {
    lua_State *L = callback_ctx_->L;
	int trace = 1;
	int r;
	lua_pushvalue(L,2);
    auto sock = conn->get_socket();
    lua_pushinteger(L, sock->get_fd());

    auto buffer = sock->get_rbuffer();
    int data_len = buffer->buffer_len();
    std::vector<char> data(data_len);
    data.resize(data_len);
    buffer->buffer_remove(data.data(), data_len);
    lua_pushlstring(L, data.data(), data_len);
    lua_pushinteger(L, data_len);
	r = lua_pcall(L, 3, 0 , trace);
	if (r == LUA_OK) {
		return;
	}

	// log_error("xpcall: {}", lua_tostring(L, -1));
    std::cout << "xpcall: " << lua_tostring(L, -1) << std::endl;
	lua_pop(L,1);
}

int server::push(int fd, const char* data, int len) {
    if (fd > connect_pool_.capacity()) {
        return -1;
    }

    auto conn = connect_pool_[fd];
    if (conn) {
        conn->write(const_cast<char*>(data), len);
        poller_->mod_event(fd, static_cast<int>(net_event_type::EVENT_WRITE) | static_cast<int>(net_event_type::EVENT_READ));
        return 0;
    }

    return -1;
}