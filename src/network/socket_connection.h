#pragma once

#include <algorithm>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <boost/asio.hpp>

#include "status.hpp"
#include "utils/logger2.hpp"

namespace ubiquant {

using boost::asio::generic::stream_protocol;

class SocketServer;

/**
 * @brief SocketConnection handles the socket connection in ubiquant
 *
 */
class SocketConnection : public std::enable_shared_from_this<SocketConnection> {
public:
    SocketConnection(stream_protocol::socket socket, SocketServer* socket_server_ptr, int conn_id);

    void start();

    // Invoke internal do_stop, and set the running status to false.
    void stop();

private:
    void do_read_header();

    void do_read_body();

    // Return should be exit after this message.
    // @return Returns true if stop the client and close the connection (i.e.,
    // handling a ExitRequest), otherwise false.
    bool process_message(MSG_CODE code, const std::string& message_in);

    void do_write(const std::string& buf);

    void do_async_write(const std::string& buf);

    // Being called when the encounter a socket error (in read/write), or by external "conn->stop()".
    // Just do some clean up and won't remove connecion from parent's pool.
    void do_stop();

    stream_protocol::socket socket;
    SocketServer* socket_server_ptr;
    int conn_id;
    bool running;

    boost::asio::streambuf buf;

    std::unordered_set<int> used_fds;

    size_t read_msg_header;
    std::string read_msg_body;
};

using socket_message_queue_t = std::deque<std::string>;

}  // namespace ubiquant
