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
#include <chrono>

#include "common/config.h"

// utils
#include "utils/logger2.hpp"
#include "utils/assertion.hpp"

namespace ubiquant {

extern volatile bool after_reset;

class MessageSender {
private:
    zmq::context_t context;
    std::string src_addr;
    std::string dst_addr;
    std::pair<int, int> channel;
    zmq::socket_t* sender;

    pthread_spinlock_t *send_locks;

    bool connected = false;

public:
    MessageSender(std::string my_addr, std::string receiver_addr, std::pair<int, int> port_pair) 
        : context(1), src_addr(my_addr), dst_addr(receiver_addr), channel(port_pair) {
        // new socket
        sender = new zmq::socket_t(context, ZMQ_PUSH);
    }

    ~MessageSender() {
        if (sender) {
            delete sender;
        }
    }

    void reset_port(std::pair<int, int> port_pair) {
        char address[64] = "";
        //snprintf(address, 32, "tcp://%s:%d", dst_addr.c_str(), channel.second);
        snprintf(address, 64, "tcp://%s:%d;%s:%d", 
                 src_addr.c_str(), 
                 channel.first, 
                 dst_addr.c_str(), 
                 channel.second);
        sender->disconnect(address);

        connected = false;

        // set new channel
        channel = port_pair;
    }

    bool send(const std::string &str) {
        zmq::message_t msg(str.length());
        memcpy((void *)msg.data(), str.c_str(), str.length());

        if (!connected) {
            // connect on-demand
            char address[64] = "";
            //snprintf(address, 32, "tcp://%s:%d", dst_addr.c_str(), channel.second);
            snprintf(address, 64, "tcp://%s:%d;%s:%d", 
                 src_addr.c_str(), 
                 channel.first, 
                 dst_addr.c_str(), 
                 channel.second);
            std::cout << "Tring to Connect to " << address << std::endl;
            sender->connect(address);
            std::cout << "Connect to " << address << std::endl;

            connected = true;
        }

        // TODO: use two channels
        bool result = sender->send(msg, ZMQ_DONTWAIT);
        // if (!result) {
        //     logstream(LOG_INFO) << "failed to send msg to ["
        //                          << dst_addr << ":" << channels[0].second << "] "
        //                          << strerror(errno) << LOG_endl;
        // }

        return result;
    }
};

} // namespace ubiquant