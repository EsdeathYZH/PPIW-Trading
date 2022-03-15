#pragma once

#include <memory>
#include <sstream> 
#include <string>
#include <utility>
#include <vector>

#include "status.hpp"

#include "common/config.h"

#include "utils/logger2.hpp"

namespace ubiquant {

class SocketServer {
public:
    SocketServer();

    ~SocketServer();

    void serve();

    std::pair<bool, std::string> process_message(MSG_CODE code, const std::string& message_in);

    // Check if @port@ exists in the connection pool.
    bool exists_connection(int port) const;

    // Remove @port@ from connection pool, before removing, the "stop"
    // on the connection has already been called.
    void remove_connection(int port);

    // Invoke the "stop" on the connection, and then remove it from the connection pool.
    void close_connection(int port);

private:
    void do_accept();

    // Call "stop" on all connections, then clear the connection pool.
    void stop();

    uint32_t port;

    boost::asio::io_context context;

    boost::asio::ip::tcp::acceptor acceptor;
    boost::asio::ip::tcp::socket socket;

    using ctx_guard = boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;
    ctx_guard guard;

    // port -> socket connection
    std::unordered_map<int, std::shared_ptr<SocketConnection>> connections;
    mutable std::mutex connections_mutx;  // protect connections in removing
};

}  // namespace ubiquant
