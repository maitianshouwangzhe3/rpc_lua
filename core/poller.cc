
#include "poller.h"

#include <unistd.h>
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

int poller::add_event(int fd, int event, bool iset) {
    epoll_event ev;
    ev.data.fd = fd;
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