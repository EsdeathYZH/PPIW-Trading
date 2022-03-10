#pragma once

#include <zmq.hpp>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <iostream>
#include <unordered_map>
#include <string>
#include <fstream>
#include <sstream>


#include "common/config.hpp"

// utils
#include "utils/logger2.hpp"
#include "utils/assertion.hpp"

namespace ubiquant {

class MessageReceiver {
private:
    std::vector<int> ports;
    std::unordered_map<int, zmq::socket_t*> receivers;     // static allocation

    zmq::context_t context;

public:
    MessageReceiver(std::vector<int> receiver_ports)
        : context(1), ports(receiver_ports) {

        for (auto port : receiver_ports) {
            receivers[port] = new zmq::socket_t(context, ZMQ_PULL);
            char address[32] = "";
            snprintf(address, 32, "tcp://*:%d", port);
            receivers[port]->bind(address);
        }
    }

    ~MessageReceiver() {
        for (auto &r : receivers)
            if (r.second) delete r.second;
    }

    std::string recv() {
        zmq::message_t msg;

        if (receivers[ports[0]]->recv(&msg) < 0) {
            logstream(LOG_ERROR) << "failed to recv msg ("
                                 << strerror(errno) << ")" << LOG_endl;
            assert(false);
        }

        return std::string((char *)msg.data(), msg.size());
    }


    bool tryrecv(std::string &str) {
        zmq::message_t msg;
        bool success = false;

        if (success = receivers[ports[0]]->recv(&msg, ZMQ_NOBLOCK))
            str = std::string((char *)msg.data(), msg.size());

        return success;
    }

};

} // namespace ubiquant