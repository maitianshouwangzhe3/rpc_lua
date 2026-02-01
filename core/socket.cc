
#include "socket.h"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>

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
    return ::bind(fd_, (struct sockaddr *)&addr, sizeof(addr)) == 0;
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
    return ::bind(fd_, (struct sockaddr *)&addr, sizeof(addr)) == 0;
}

int socket::listen() {
    if (fd_ <= 0) {
        return -1;
    }

    return ::listen(fd_, 5);
}

void socket::close() {
    if (fd_ > 0) {
        ::close(fd_);
        fd_ = -1;
    }
}

void socket::set_fd_non_block() {
    int flags = fcntl(fd_, F_GETFL, 0);
    fcntl(fd_, F_SETFL, flags | O_NONBLOCK);
}

int socket::recv() {
    int ret = 0;
    do {
        char buffer[4096] = {0};
        int nready = ::recv(fd_, buffer, 4096, 0);
        if (nready == 0) {
            return 0;
        } else if (nready < 0) {
            if (errno == EINTR)
                continue;
            if (errno == EWOULDBLOCK) {
                break;
            }
        } else {
            rbuffer->buffer_add(buffer, nready);
            ret += nready;
        }
    } while (true);
    return ret;
}

int socket::read() {
    int ret = 0;
    do {
        char buffer[4096] = {0};
        int nready = ::read(fd_, buffer, 4096);
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
    int ret = 0;
    do {
        char buffer[4096] = {0};
        int len = wbuffer->buffer_remove(buffer, 4096);
        int size = 0;
        if (len == 4096) {
            size = ::send(fd_, buffer, len, 0);
        } else if (len < 4096 && len > 0) {
            size = ::send(fd_, buffer, len, 0);
            break;
        } else {
            break;
        }

        if (size < 0) {
            if (errno == EINTR)
                continue;
            if (errno == EWOULDBLOCK) {
                break;
            }
        }
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
    return ::accept(fd_, NULL, NULL);
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
    return ::connect(fd_, (const struct sockaddr*)&addr, len);
}

int socket::no_delay() {
    int value = 1;
    return setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, &value, sizeof(value));
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

    fd_ = ::socket(AF_INET, type, 0);
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