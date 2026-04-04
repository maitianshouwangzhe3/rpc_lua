
#include "poller.h"
#include "socket.h"
#include "net_compat.h"

#ifdef _WIN32
#include <unordered_map>
#include <cstring>

enum { IOCP_OP_READ = 0, IOCP_OP_WRITE = 1 };

struct iocp_per_io {
    WSAOVERLAPPED overlapped;
    WSABUF wsabuf;
    char buffer[8192];
    int operation;
};

struct poller::iocp_state {
    HANDLE iocp_handle;
    std::unordered_map<int, socket*> socket_map;
};

static int g_wsa_init_count = 0;

static void iocp_post_recv(HANDLE iocp, SOCKET s) {
    auto* io = new iocp_per_io();
    ZeroMemory(&io->overlapped, sizeof(io->overlapped));
    io->wsabuf.buf = io->buffer;
    io->wsabuf.len = sizeof(io->buffer);
    io->operation = IOCP_OP_READ;

    DWORD flags = 0;
    DWORD bytes = 0;
    int ret = WSARecv(s, &io->wsabuf, 1, &bytes, &flags, &io->overlapped, NULL);
    if (ret == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err != WSA_IO_PENDING) {
            delete io;
        }
    }
}

static void iocp_post_send(HANDLE iocp, SOCKET s, socket* sock) {
    auto wbuf = sock->get_wbuffer();
    int len = wbuf->buffer_len();
    if (len <= 0) return;

    int copy_len = (len > (int)sizeof(iocp_per_io().buffer))
        ? (int)sizeof(iocp_per_io().buffer) : len;

    auto* io = new iocp_per_io();
    ZeroMemory(&io->overlapped, sizeof(io->overlapped));
    io->operation = IOCP_OP_WRITE;
    wbuf->buffer_remove(io->buffer, copy_len);
    io->wsabuf.buf = io->buffer;
    io->wsabuf.len = copy_len;

    DWORD bytes = 0;
    int ret = WSASend(s, &io->wsabuf, 1, &bytes, 0, &io->overlapped, NULL);
    if (ret == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err != WSA_IO_PENDING) {
            wbuf->buffer_add(io->buffer, copy_len);
            delete io;
        }
    }
}

poller::poller() : state_(nullptr) {
    if (g_wsa_init_count == 0) {
        WSADATA wsa;
        WSAStartup(MAKEWORD(2, 2), &wsa);
    }
    g_wsa_init_count++;

    state_ = new iocp_state();
    state_->iocp_handle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
}

poller::~poller() {
    if (state_) {
        CloseHandle(state_->iocp_handle);
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
    socket* sock = reinterpret_cast<socket*>(ptr);
    SOCKET s = (SOCKET)fd;

    CreateIoCompletionPort((HANDLE)s, state_->iocp_handle, (ULONG_PTR)fd, 0);
    state_->socket_map[fd] = sock;

    if (sock) {
        sock->set_iocp(true);
    }

    if (event & static_cast<int>(net_event_type::EVENT_READ)) {
        iocp_post_recv(state_->iocp_handle, s);
    }

    return 0;
}

int poller::mod_event(int fd, int event, bool iset) {
    if (event & static_cast<int>(net_event_type::EVENT_WRITE)) {
        auto it = state_->socket_map.find(fd);
        if (it != state_->socket_map.end() && it->second) {
            iocp_post_send(state_->iocp_handle, (SOCKET)fd, it->second);
        }
    }
    return 0;
}

int poller::del_event(int fd) {
    state_->socket_map.erase(fd);
    CancelIoEx((HANDLE)(SOCKET)fd, NULL);
    return 0;
}

int poller::poll(std::vector<net_event>& evs, int time_out) {
    std::unordered_map<int, int> event_map;

    while (true) {
        DWORD bytes = 0;
        ULONG_PTR key = 0;
        LPOVERLAPPED pov = NULL;

        BOOL ok = GetQueuedCompletionStatus(
            state_->iocp_handle, &bytes, &key, &pov, (DWORD)time_out);

        if (!ok && pov == NULL) {
            break;
        }

        if (pov == NULL) continue;

        iocp_per_io* pio = (iocp_per_io*)pov;
        int fd = (int)key;
        auto it = state_->socket_map.find(fd);

        if (pio->operation == IOCP_OP_READ) {
            if (it == state_->socket_map.end()) {
                delete pio;
                continue;
            }

            if (bytes == 0 || !ok) {
                event_map[fd] |= static_cast<int>(net_event_type::EVENT_RDHUP);
                delete pio;
            } else {
                socket* sock = it->second;
                sock->get_rbuffer()->buffer_add(pio->buffer, bytes);
                event_map[fd] |= static_cast<int>(net_event_type::EVENT_READ);
                iocp_post_recv(state_->iocp_handle, (SOCKET)fd);
                delete pio;
            }
        } else if (pio->operation == IOCP_OP_WRITE) {
            if (it == state_->socket_map.end()) {
                delete pio;
                continue;
            }
            event_map[fd] |= static_cast<int>(net_event_type::EVENT_WRITE);
            delete pio;
        }

        time_out = 0;
    }

    for (auto& kv : event_map) {
        net_event ev;
        ev.fd = kv.first;
        ev.event = kv.second;
        ev.ptr = nullptr;
        evs.push_back(ev);
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
