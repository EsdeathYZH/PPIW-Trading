#include "socket_connection.h"
#include "socket_server.h"

namespace ubiquant {

SocketServer::SocketServer()
    : acceptor(context),
      socket(context),
      guard(boost::asio::make_work_guard(context)) {

    this->port = Config::server_port_base;

    // bind and listen
    auto endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port);
    acceptor.open(endpoint.protocol());
    using reuse_port = boost::asio::detail::socket_option::boolean<SOL_SOCKET, SO_REUSEPORT>;
    // reuse address and port for rpc service.
    acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    acceptor.set_option(reuse_port(true));
    acceptor.bind(endpoint);
    acceptor.listen();

    std::cout << "Server will listen on port: " << port << std::endl;
}

SocketServer::~SocketServer() {
    guard.reset();
    stop();

    if (acceptor.is_open()) {
        acceptor.close();
    }

    // stop the boost::asio context at last
    context.stop();
}

void SocketServer::serve() {
    do_accept();
    context.run();
}

std::pair<bool, std::string>
SocketServer::process_message(MSG_CODE code, const std::string& message_in) {
    std::string message_out;
    switch (code) {
    case MSG_CODE::INFO_RPC:
        // TODO:
        break;
    case MSG_CODE::EXIT_RPC:
        return std::make_pair(true, std::string());
    }
    return std::make_pair(false, message_out);
}

// Check if @port@ exists in the connection pool.
bool SocketServer::exists_connection(int port) const {
    std::lock_guard<std::mutex> scope_lock(this->connections_mutx);
    return connections.find(port) != connections.end();
}

// Remove @port@ from connection pool, before removing, the "stop"
// on the connection has already been called.
void SocketServer::remove_connection(int port) {
    std::lock_guard<std::mutex> scope_lock(this->connections_mutx);
    connections.erase(port);
}

// Invoke the "stop" on the connection, and then remove it from the connection pool.
void SocketServer::close_connection(int port) {
    std::lock_guard<std::mutex> scope_lock(this->connections_mutx);
    connections.at(port)->stop();
    remove_connection(port);
}

void SocketServer::do_accept() {
    if (!acceptor.is_open()) {
        std::cout << "Acceptor is not open..." << std::endl;
        return;
    }
    acceptor.async_accept(socket, [this](boost::system::error_code ec) {
        if (!ec) {
            std::shared_ptr<SocketConnection> conn = 
                std::make_shared<SocketConnection>(std::move(this->socket), this, this->port);
            conn->start();
            std::lock_guard<std::mutex> scope_lock(this->connections_mutx);
            connections.emplace(this->port, conn);
        }
        do_accept();
    });
}

// Call "stop" on all connections, then clear the connection pool.
void SocketServer::stop() {
    std::lock_guard<std::mutex> scope_lock(this->connections_mutx);
    for (auto& pair : connections) {
        pair.second->stop();
    }
    connections.clear();
}

}  // namespace ubiquant
