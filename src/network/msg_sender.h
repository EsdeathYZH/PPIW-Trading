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

class MessageSender {
private:
    std::vector<int> ports;
    std::unordered_map<int, zmq::socket_t*> senders;
    zmq::context_t context;

public:
    MessageSender(std::string receiver_addr, std::vector<int> receiver_ports) 
        : context(1), ports(receiver_ports) {
        // set send socket
        for(auto port : receiver_ports) {
            // new socket on-demand
            char address[32] = "";
            snprintf(address, 32, "tcp://%s:%d", receiver_addr.c_str(), port);
            senders[port] = new zmq::socket_t(context, ZMQ_PUSH);
            senders[port]->connect(address);
        }
    }

    ~MessageSender() {
        for (auto &s : senders) {
            if (s.second) {
                delete s.second;
            }
        }
    }

    bool send(const std::string &str) {
        zmq::message_t msg(str.length());
        memcpy((void *)msg.data(), str.c_str(), str.length());

        int result = senders[ports[0]]->send(msg, ZMQ_DONTWAIT);
        if (result < 0) {
            logstream(LOG_ERROR) << "failed to send msg to ["
                                 << dst_sid << ", " << dst_tid << "] "
                                 << strerror(errno) << LOG_endl;
        }

        return result;
    }
};

} // namespace ubiquant