#include "socket_connection.h"
#include "socket_server.h"

namespace ubiquant {

SocketConnection::SocketConnection(stream_protocol::socket socket, SocketServer* socket_server_ptr, int conn_id)
    : socket(std::move(socket)),
      socket_server_ptr(socket_server_ptr),
      conn_id(conn_id),
      running(false) {}

void SocketConnection::start() {
    running = true;
    do_read_header();
}

void SocketConnection::stop() {
    do_stop();
    running = false;
}

void SocketConnection::do_read_header() {
    auto self(this->shared_from_this());
    boost::asio::async_read(socket, boost::asio::buffer(&read_msg_header, sizeof(uint64_t)),
                            [this, self](boost::system::error_code ec, std::size_t) {
                                if (!ec && running) {
                                    do_read_body();
                                } else {
                                    do_stop();
                                    socket_server_ptr->remove_connection(conn_id);
                                    return;
                                }
                            });
}

void SocketConnection::do_read_body() {
    uint32_t msg_size = read_msg_header & 0xffffffff;
    MSG_CODE code = static_cast<MSG_CODE>(read_msg_header >> 32);
    if (msg_size > 64 * 1024 * 1024) {  // 64M bytes
        // We set a hard limit for the message buffer size, since an evil client,
        // e.g., telnet.
        //
        // We don't revise the structure of protocol, for backwards compatible, as
        // we already released wheel packages on pypi.
        do_stop();
        socket_server_ptr->remove_connection(conn_id);
        return;
    }
    read_msg_body.resize(msg_size);
    auto self(shared_from_this());
    boost::asio::async_read(socket, boost::asio::buffer(&read_msg_body[0], msg_size),
                            [this, self, code](boost::system::error_code ec, std::size_t) {
                                if ((!ec || ec == boost::asio::error::eof) && running) {
                                    bool exit = process_message(code, read_msg_body);
                                    if (exit || ec == boost::asio::error::eof) {
                                        do_stop();
                                        socket_server_ptr->remove_connection(conn_id);
                                        return;
                                    }
                                } else {
                                    do_stop();
                                    socket_server_ptr->remove_connection(conn_id);
                                    return;
                                }
                                // start next-round read
                                do_read_header();
                            });
}

bool SocketConnection::process_message(MSG_CODE code, const std::string& message_in) {
    auto result = socket_server_ptr->process_message(code, message_in);
    if (!result.first) do_write(result.second);
    return result.first;
}

void SocketConnection::do_write(const std::string& buf) {
    std::string to_send;
    size_t length = buf.size();
    to_send.resize(length + sizeof(uint32_t));
    char* ptr = &to_send[0];
    memcpy(ptr, &length, sizeof(size_t));
    ptr += sizeof(uint32_t);
    memcpy(ptr, buf.data(), length);
    do_async_write(to_send);
}

void SocketConnection::do_async_write(const std::string& buf) {
    auto self(shared_from_this());
    boost::asio::async_write(socket, boost::asio::buffer(buf.data(), buf.length()),
                             [this, self](boost::system::error_code ec, std::size_t) {
                                 if (ec) {
                                     do_stop();
                                     socket_server_ptr->remove_connection(conn_id);
                                 }
                             });
}

void SocketConnection::do_stop() {
    // On Mac the state of socket may be "not connected" after the client has
    // already closed the socket, hence there will be an exception.
    boost::system::error_code ec;
    socket.shutdown(stream_protocol::socket::shutdown_both, ec);
    socket.close();
    logstream(LOG_DEBUG) << "Close a connection" << LOG_endl;
}

}  // namespace ubiquant
