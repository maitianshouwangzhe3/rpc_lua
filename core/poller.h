#pragma once

#include <vector>
#include <cstdint>

namespace rpc_lua {
    class socket;
}

enum class net_event_type {
    EVENT_READ  = 1 << 1,
    EVENT_WRITE = 1 << 2,
    EVENT_RDHUP = 1 << 3
};

struct net_event {
    int fd;
    int event;
    void* ptr;

    net_event(): fd(-1), event(0), ptr(nullptr) {}
};


class poller final {
public:
    poller();
    ~poller();

    int poll(std::vector<net_event>& evs, int time_out = -1);

    int add_event(int fd, int event, bool iset = true, void* ptr = nullptr);

    int mod_event(int fd, int event, bool iset = true);

    int del_event(int fd);

private:
    uint32_t add_event(int event, bool iset);

#ifdef _WIN32
    struct iocp_state;
    iocp_state* state_;
#else
    int epfd_;
#endif
};
