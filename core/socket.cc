
#include "socket.h"

#include "net_compat.h"

#include <cerrno>
#include <cstring>

socket::socket() {
    init(protocol::TCP);
    rbuffer = std::make_shared<chain_buffer>();
    wbuffer = std::make_shared<chain_buffer>();
}

socket::~socket() {

}

socket::socket(int fd): fd_(fd) {
    rbuffer = std::make_shared<chain_buffer>();
    wbuffer = std::make_shared<chain_buffer>();
}

socket::socket(protocol proto) {
    init(proto);
    rbuffer = std::make_shared<chain_buffer>();
    wbuffer = std::make_shared<chain_buffer>();
}

socket::socket(int fd, protocol proto): fd_(fd) {
    init(proto);
    rbuffer = std::make_shared<chain_buffer>();
    wbuffer = std::make_shared<chain_buffer>();
}

bool socket::bind(int port) {
    if (fd_ <= 0) {
        return false;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    return ::bind((net_socket_t)fd_, (struct sockaddr *)&addr, sizeof(addr)) == 0;
}

// 新增：支持 IP 和端口绑定的方法
bool socket::bind(const std::string& ip, int port) {
    if (fd_ <= 0) {
        return false;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);  // 使用传入的 IP 地址
    return ::bind((net_socket_t)fd_, (struct sockaddr *)&addr, sizeof(addr)) == 0;
}

int socket::listen() {
    if (fd_ <= 0) {
        return -1;
    }

    return ::listen((net_socket_t)fd_, 5);
}

void socket::close() {
    if (fd_ > 0) {
        NET_CLOSE_SOCKET((net_socket_t)fd_);
        fd_ = -1;
    }
}

void socket::set_fd_non_block() {
#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket((SOCKET)fd_, FIONBIO, &mode);
#else
    int flags = fcntl(fd_, F_GETFL, 0);
    fcntl(fd_, F_SETFL, flags | O_NONBLOCK);
#endif
}

int socket::recv() {
    // IOCP 模式下，数据已由完成端口放入 rbuffer
    if (is_iocp_) {
        return rbuffer->buffer_len();
    }

    int ret = 0;
    do {
        char buffer[4096] = {0};
#ifdef _WIN32
        int nready = ::recv((SOCKET)fd_, buffer, 4096, 0);
        if (nready == 0) {
            return 0;
        } else if (nready == SOCKET_ERROR) {
            int err = WSAGetLastError();
            if (err == WSAEINTR)
                continue;
            if (err == WSAEWOULDBLOCK) {
                break;
            }
            return -1;
        }
#else
        int nready = ::recv(fd_, buffer, 4096, 0);
        if (nready == 0) {
            return 0;
        } else if (nready < 0) {
            if (errno == EINTR)
                continue;
            if (errno == EWOULDBLOCK) {
                break;
            }
        }
#endif
        rbuffer->buffer_add(buffer, nready);
        ret += nready;
    } while (true);
    return ret;
}

int socket::read() {
    // IOCP 模式下，数据已由完成端口放入 rbuffer
    if (is_iocp_) {
        return rbuffer->buffer_len();
    }

    int ret = 0;
    do {
        char buffer[4096] = {0};
#ifdef _WIN32
        int nready = ::recv((SOCKET)fd_, buffer, 4096, 0);
#else
        int nready = ::read(fd_, buffer, 4096);
#endif
        if (nready >= 4096) {
            rbuffer->buffer_add(buffer, nready);
            ret += nready;
            continue;
        } else {
            rbuffer->buffer_add(buffer, nready);
            ret += nready;
        }

        break;
    } while (true);

    return ret;
}

int socket::wbuffer_len() {
    return wbuffer->buffer_len();
}

int socket::rbuffer_len() {
    return rbuffer->buffer_len();
}

int socket::put(void* data, uint32_t datlen) {
    return rbuffer->buffer_remove(data, datlen);
}

int socket::send() {
    // IOCP 模式下，发送由完成端口异步处理
    if (is_iocp_) {
        return wbuffer->buffer_len();
    }

    int ret = 0;
    do {
        char buffer[4096] = {0};
        int len = wbuffer->buffer_remove(buffer, 4096);
        int size = 0;
        if (len == 4096) {
            size = ::send((net_socket_t)fd_, buffer, len, 0);
        } else if (len < 4096 && len > 0) {
            size = ::send((net_socket_t)fd_, buffer, len, 0);
            break;
        } else {
            break;
        }

#ifdef _WIN32
        if (size == SOCKET_ERROR) {
            int err = WSAGetLastError();
            if (err == WSAEINTR)
                continue;
            if (err == WSAEWOULDBLOCK) {
                break;
            }
        }
#else
        if (size < 0) {
            if (errno == EINTR)
                continue;
            if (errno == EWOULDBLOCK) {
                break;
            }
        }
#endif
        ret += size;
    } while (true);
    return ret;
}

int socket::push(void* data, uint32_t datlen) {
    return wbuffer->buffer_add(data, datlen);
}

int socket::get_fd() {
    return fd_;
}

int socket::accept() {
#ifdef _WIN32
    SOCKET s = ::accept((SOCKET)fd_, NULL, NULL);
    if (s == INVALID_SOCKET) {
        return -1;
    }
    return (int)s;
#else
    return ::accept(fd_, NULL, NULL);
#endif
}

int socket::set_socket(int fd) {
    fd_ = fd;
    return fd_;
}

int socket::create_socket() {
    init(proto_);
    return fd_;
}

int socket::connect(const char* ip, int port) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &addr.sin_addr);
    socklen_t len = sizeof(addr);
    return ::connect((net_socket_t)fd_, (const struct sockaddr*)&addr, len);
}

int socket::no_delay() {
    int value = 1;
    return setsockopt((net_socket_t)fd_, IPPROTO_TCP, TCP_NODELAY,
#ifdef _WIN32
        (const char*)&value,
#else
        &value,
#endif
        sizeof(value));
}

void socket::init(protocol proto) {
    int type = -1;
    switch (proto) {
        case protocol::TCP:
            type = SOCK_STREAM;
            break;
        case protocol::UDP:
            type = SOCK_DGRAM;
            break;
        default:
            type = SOCK_STREAM;
            break;
    }

    fd_ = (int)::socket(AF_INET, type, 0);
#ifdef _WIN32
    if (fd_ == (int)INVALID_SOCKET) {
        fd_ = -1;
    }
#endif
}

std::shared_ptr<chain_buffer> socket::get_rbuffer() {
    return rbuffer;
}

std::shared_ptr<chain_buffer> socket::get_wbuffer() {
    return wbuffer;
}

int socket::check_sep_rbuffer(const char* sep, const int seplen) {
    return rbuffer->buffer_search(sep, seplen);
}
