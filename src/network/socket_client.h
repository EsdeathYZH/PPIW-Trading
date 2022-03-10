#pragma once

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "status.hpp"

#include "common/config.hpp"

#include "utils/errors.hpp"
#include "utils/assertion.hpp"
#include "utils/logger2.hpp"

namespace ubiquant {

class SocketClient {
public:
    SocketClient();

    ~SocketClient();

    /**
     * @brief Connect to server using the given TCP `host` and `port`.
     *
     * @param host The host of server.
     * @param port The TCP port of server's service.
     *
     * @return Status that indicates whether the connect has succeeded.
     */
    Status connect_to_server(const std::string& host, uint32_t port);

    bool connected() const;

    void disconnect();

    Status retrieve_cluster_info();

protected:
    int kNumConnectAttempts = 5;
    int kConnectTimeoutMs = 1000;

    mutable bool is_connected;
    std::string rpc_endpoint;
    int socket_fd;

    // A mutex which protects the client.
    std::recursive_mutex client_mutex;

    Status connect_rpc_socket(const std::string& host, uint32_t port, int& socket_fd);

    Status connect_rpc_socket_retry(const std::string& host, const uint32_t port, int& socket_fd);

    Status send_bytes(int fd, const void* data, uint32_t length);

    Status send_message(int fd, MSG_CODE code, const std::string& msg);

    Status recv_bytes(int fd, void* data, size_t length);

    Status recv_message(int fd, std::string& msg);
};

}  // namespace ubiquant
