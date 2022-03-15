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

#include "common/config.h"

// utils
#include "utils/logger2.hpp"
#include "utils/assertion.hpp"

namespace ubiquant {

class MessageSender {
private:
    zmq::context_t context;
    std::string addr;
    std::vector<std::pair<int, int>> channels;
    std::unordered_map<int, zmq::socket_t*> senders;

public:
    MessageSender(std::string receiver_addr, std::vector<std::pair<int, int>> port_pairs) 
        : context(1), addr(receiver_addr), channels(port_pairs) {
        // set send socket
        for(auto [src_port, dst_port] : channels) {
            // new socket on-demand
            char address[32] = "";
            snprintf(address, 32, "tcp://%s:%d", receiver_addr.c_str(), dst_port);
            senders[dst_port] = new zmq::socket_t(context, ZMQ_PUSH);
            senders[dst_port]->connect(address);
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

        // TODO: use two channels
        int result = senders[channels[0].second]->send(msg, ZMQ_DONTWAIT);
        if (result < 0) {
            logstream(LOG_ERROR) << "failed to send msg to ["
                                 << addr << ":" << channels[0].second << "] "
                                 << strerror(errno) << LOG_endl;
        }

        return result;
    }
};

} // namespace ubiquant