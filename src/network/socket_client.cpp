#include "socket_client.h"

namespace ubiquant {

// return the status if the status is not OK.
#ifndef RETURN_ON_ERROR
#define RETURN_ON_ERROR(status) \
    do {                        \
        auto _ret = (status);   \
        if (!_ret.ok()) {       \
            return _ret;        \
        }                       \
    } while (0)
#endif  // RETURN_ON_ERROR

// return the status if the status is not OK.
#ifndef RETURN_ON_ASSERT_NO_VERBOSE
#define RETURN_ON_ASSERT_NO_VERBOSE(condition)          \
    do {                                                \
        if (!(condition)) {                             \
            return Status::AssertionFailed(#condition); \
        }                                               \
    } while (0)
#endif  // RETURN_ON_ASSERT_NO_VERBOSE

// return the status if the status is not OK.
#ifndef RETURN_ON_ASSERT_VERBOSE
#define RETURN_ON_ASSERT_VERBOSE(condition, message)     \
    do {                                                 \
        if (!(condition)) {                              \
            return Status::AssertionFailed(              \
                std::string(#condition ": ") + message); \
        }                                                \
    } while (0)
#endif  // RETURN_ON_ASSERT_VERBOSE

#ifndef RETURN_ON_ASSERT
#define RETURN_ON_ASSERT(...)                        \
    GET_MACRO(__VA_ARGS__, RETURN_ON_ASSERT_VERBOSE, \
              RETURN_ON_ASSERT_NO_VERBOSE)           \
    (__VA_ARGS__)
#endif  // RETURN_ON_ASSERT

SocketClient::SocketClient() : is_connected(false), socket_fd(0) {}

SocketClient::~SocketClient() { disconnect(); }

/**
 * @brief Connect to server using the given TCP `host` and `port`.
 *
 * @param host The host of server.
 * @param port The TCP port of server's service.
 *
 * @return Status that indicates whether the connect has succeeded.
 */
Status SocketClient::connect_to_server(const std::string& host, uint32_t port) {
    std::lock_guard<std::recursive_mutex> guard(client_mutex);
    std::string connect_rpc_endpoint = host + ":" + std::to_string(port);
    logstream(LOG_INFO) << "Try to connect to " << connect_rpc_endpoint << LOG_endl;
    ASSERT(!is_connected || connect_rpc_endpoint == this->rpc_endpoint);
    if (is_connected) {
        return Status::OK();
    }
    this->rpc_endpoint = connect_rpc_endpoint;
    RETURN_ON_ERROR(connect_rpc_socket_retry(host, port, socket_fd));

    is_connected = true;

    return Status::OK();
}

bool SocketClient::connected() const {
    if (is_connected && recv(socket_fd, NULL, 1, MSG_PEEK | MSG_DONTWAIT) != -1) {
        is_connected = false;
    }
    return is_connected;
}

void SocketClient::disconnect() {
    std::lock_guard<std::recursive_mutex> __guard(this->client_mutex);
    if (!this->is_connected) {
        return;
    }
    std::string exit_msg;
    send_message(this->socket_fd, MSG_CODE::EXIT_RPC, "");
    close(socket_fd);
    is_connected = false;
}

Status SocketClient::retrieve_cluster_info() {
    RETURN_ON_ERROR(send_message(this->socket_fd, MSG_CODE::INFO_RPC, "test"));

    std::string reply_msg;
    RETURN_ON_ERROR(recv_message(this->socket_fd, reply_msg));
    // show cluster info
    std::cout << "Reply:" << reply_msg << std::endl;
    return Status::OK();
}

Status SocketClient::connect_rpc_socket(const std::string& host, uint32_t port, int& socket_fd) {
    std::string port_string = std::to_string(port);

    struct addrinfo hints = {}, *addrs;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(host.c_str(), port_string.c_str(), &hints, &addrs) != 0) {
        return Status::IOError("getaddrinfo() failed for endpoint " + host + ":" +
                                std::to_string(port));
    }

    socket_fd = -1;
    for (struct addrinfo* addr = addrs; addr != nullptr; addr = addr->ai_next) {
        socket_fd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        if (socket_fd == -1) {
            continue;
        }
        if (connect(socket_fd, addr->ai_addr, addr->ai_addrlen) != 0) {
            continue;
        }
        break;
    }
    freeaddrinfo(addrs);
    if (socket_fd == -1) {
        return Status::IOError("socket/connect failed for endpoint " + host + ":" +
                                std::to_string(port));
    }

    return Status::OK();
}

Status SocketClient::connect_rpc_socket_retry(const std::string& host, const uint32_t port, int& socket_fd) {
    int num_retries = kNumConnectAttempts;
    int64_t timeout = kConnectTimeoutMs;

    auto status = connect_rpc_socket(host, port, socket_fd);

    while (!status.ok() && num_retries > 0) {
        std::cout << "Connection to RPC socket failed for endpoint " << host << ":"
                    << port << " with ret = " << status.get_msg() << ", retrying " << num_retries
                    << " more times." << std::endl;
        usleep(static_cast<int>(timeout * 1000));
        status = connect_rpc_socket(host, port, socket_fd);
        --num_retries;
    }
    if (!status.ok()) {
        status = Status::ConnectionFailed();
    } else {
        std::cout << "Connect successfuly." << std::endl;
    }
    return status;
}

Status SocketClient::send_bytes(int fd, const void* data, uint32_t length) {
    ssize_t nbytes = 0;
    size_t bytes_left = length;
    size_t offset = 0;
    const char* ptr = static_cast<const char*>(data);
    while (bytes_left > 0) {
        nbytes = write(fd, ptr + offset, bytes_left);
        if (nbytes < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                continue;
            }
            return Status::IOError("Send message failed: " +
                                    std::string(strerror(errno)));
        } else if (nbytes == 0) {
            return Status::IOError("Send message failed: encountered unexpected EOF");
        }
        bytes_left -= nbytes;
        offset += nbytes;
    }
    return Status::OK();
}

Status SocketClient::send_message(int fd, MSG_CODE code, const std::string& msg) {
    uint32_t length = msg.length();
    RETURN_ON_ERROR(send_bytes(fd, &length, sizeof(uint32_t)));
    RETURN_ON_ERROR(send_bytes(fd, &code, sizeof(uint32_t)));
    RETURN_ON_ERROR(send_bytes(fd, msg.data(), length));
    return Status::OK();
}

Status SocketClient::recv_bytes(int fd, void* data, size_t length) {
    ssize_t nbytes = 0;
    size_t bytes_left = length;
    size_t offset = 0;
    char* ptr = static_cast<char*>(data);
    while (bytes_left > 0) {
        nbytes = read(fd, ptr + offset, bytes_left);
        if (nbytes < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                continue;
            }
            return Status::IOError("Receive message failed: " +
                                    std::string(strerror(errno)));
        } else if (nbytes == 0) {
            return Status::IOError(
                "Receive message failed: encountered unexpected EOF");
        }
        bytes_left -= nbytes;
        offset += nbytes;
    }
    return Status::OK();
}

Status SocketClient::recv_message(int fd, std::string& msg) {
    uint32_t length;
    RETURN_ON_ERROR(recv_bytes(fd, &length, sizeof(uint32_t)));
    // std::cout << "msg length:" << length << std::endl;
    msg.resize(length);
    RETURN_ON_ERROR(recv_bytes(fd, &msg[0], length));
    return Status::OK();
}

}  // namespace ubiquant
