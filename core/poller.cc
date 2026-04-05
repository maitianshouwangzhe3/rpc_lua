
#include "poller.h"
#include "socket.h"
#include "net_compat.h"

#ifdef _WIN32
#include <unordered_map>
#include <cstring>
#include <winsock2.h>

struct poller::select_state {
    std::unordered_map<int, int> event_map;
};

static int g_wsa_init_count = 0;

poller::poller() : state_(nullptr) {
    if (g_wsa_init_count == 0) {
        WSADATA wsa;
        (void)WSAStartup(MAKEWORD(2, 2), &wsa);
    }
    g_wsa_init_count++;

    state_ = new select_state();
}

poller::~poller() {
    if (state_) {
        delete state_;
    }
    g_wsa_init_count--;
    if (g_wsa_init_count == 0) {
        WSACleanup();
    }
}

uint32_t poller::add_event(int event, bool iset) {
    return 0;
}

int poller::add_event(int fd, int event, bool iset, void* ptr) {
    state_->event_map[fd] = event;
    return 0;
}

int poller::mod_event(int fd, int event, bool iset) {
    state_->event_map[fd] = event;
    return 0;
}

int poller::del_event(int fd) {
    state_->event_map.erase(fd);
    return 0;
}

int poller::poll(std::vector<net_event>& evs, int time_out) {
    fd_set read_fds, write_fds, except_fds;
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    FD_ZERO(&except_fds);

    int max_fd = 0;

    for (auto& kv : state_->event_map) {
        int fd = kv.first;
        int events = kv.second;

        if (events & static_cast<int>(net_event_type::EVENT_READ)) {
            FD_SET((SOCKET)fd, &read_fds);
        }

        if (events & static_cast<int>(net_event_type::EVENT_WRITE)) {
            FD_SET((SOCKET)fd, &write_fds);
        }

        if (fd > max_fd) {
            max_fd = fd;
        }
    }

    struct timeval tv;
    struct timeval* tvp = nullptr;
    if (time_out >= 0) {
        tv.tv_sec = time_out / 1000;
        tv.tv_usec = (time_out % 1000) * 1000;
        tvp = &tv;
    }

    int nready = select(max_fd + 1, &read_fds, &write_fds, &except_fds, tvp);
    if (nready <= 0) {
        return 0;
    }

    for (auto& kv : state_->event_map) {
        int fd = kv.first;
        net_event ev;
        ev.fd = fd;
        ev.ptr = nullptr;

        if (FD_ISSET((SOCKET)fd, &read_fds)) {
            ev.event |= static_cast<int>(net_event_type::EVENT_READ);
        }

        if (FD_ISSET((SOCKET)fd, &write_fds)) {
            ev.event |= static_cast<int>(net_event_type::EVENT_WRITE);
        }

        if (ev.event != 0) {
            evs.push_back(ev);
        }
    }

    return (int)evs.size();
}

#else
#include <sys/epoll.h>

poller::poller() {
    epfd_ = epoll_create1(EPOLL_CLOEXEC);
}

poller::~poller() {
    close(epfd_);
}

int poller::poll(std::vector<net_event>& evs, int time_out) {
    epoll_event events[128] = {};
    int nready = epoll_wait(epfd_, events, 128, time_out);
    if (nready > 0) {
        for (int i = 0; i < nready; i++) {
            net_event ev;
            if (events[i].events & EPOLLRDHUP || events[i].events & EPOLLHUP) {
                ev.event |= static_cast<uint32_t>(net_event_type::EVENT_RDHUP);
            } else {
                if (events[i].events & EPOLLIN) {
                    ev.event |= static_cast<uint32_t>(net_event_type::EVENT_READ);
                }

                if (events[i].events & EPOLLOUT) {
                    ev.event |= static_cast<uint32_t>(net_event_type::EVENT_WRITE);
                }
            }
            
            ev.fd = events[i].data.fd;
            ev.ptr = events[i].data.ptr;
            evs.emplace_back(std::move(ev));
        }
    }
    return evs.size();
}

uint32_t poller::add_event(int event, bool iset) {
    int ev = 0;

    if (event & static_cast<uint32_t>(net_event_type::EVENT_READ)) {
        ev |= EPOLLIN;
    }

    if (event & static_cast<uint32_t>(net_event_type::EVENT_WRITE)) {
        ev |= EPOLLOUT;
    }

    if (iset) {
        ev |= EPOLLET;
    }

    return ev;
}

int poller::add_event(int fd, int event, bool iset, void* ptr) {
    epoll_event ev;
    ev.data.fd = fd;
    ev.data.ptr = ptr;
    ev.events = add_event(event, iset);
    return epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &ev);
}

int poller::mod_event(int fd, int event, bool iset) {
    epoll_event ev;
    ev.data.fd = fd;
    ev.events = add_event(event, iset);
    return epoll_ctl(epfd_, EPOLL_CTL_MOD, fd, &ev);
}

int poller::del_event(int fd) {
    return epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, nullptr);
}

#endif
