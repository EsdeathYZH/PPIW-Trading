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

class MessageSender {
private:
    zmq::context_t context;
    std::string addr;
    std::vector<std::pair<int, int>> channels;
    std::unordered_map<int, zmq::socket_t*> senders;

    pthread_spinlock_t *send_locks;

public:
    MessageSender(std::string receiver_addr, std::vector<std::pair<int, int>> port_pairs) 
        : context(1), addr(receiver_addr), channels(port_pairs) {
        
        send_locks = (pthread_spinlock_t *)malloc(sizeof(pthread_spinlock_t) * channels.size());
        for (int i = 0; i < channels.size(); i++)
            pthread_spin_init(&send_locks[i], 0);
        
        // set send socket
        // for(auto [src_port, dst_port] : channels) {
        //     // new socket on-demand
        //     char address[32] = "";
        //     snprintf(address, 32, "tcp://%s:%d", receiver_addr.c_str(), dst_port);
        //     senders[dst_port] = new zmq::socket_t(context, ZMQ_PUSH);
        //     while(true) {
        //         senders[dst_port]->connect(address);
        //         if (errno == EAGAIN) {
        //             std::this_thread::sleep_for(std::chrono::seconds(3));
        //             std::cout << "Try to connect to " << receiver_addr << ":" << dst_port << " ..." << std::endl;
        //             continue;
        //         } else {
        //             break;
        //         }
        //     }
        //     std::cout << "Connect to " << receiver_addr << ":" << dst_port << " successfully!" << std::endl;
        // }
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

        pthread_spin_lock(&send_locks[0]);
        if (senders.find(channels[0].second) == senders.end()) {
            // new socket on-demand
            char address[32] = "";
            snprintf(address, 32, "tcp://%s:%d", addr.c_str(), channels[0].second);
            senders[channels[0].second] = new zmq::socket_t(context, ZMQ_PUSH);
            senders[channels[0].second]->connect(address);
            std::cout << "Connect to " << address << std::endl;
        }

        // TODO: use two channels
        bool result = senders[channels[0].second]->send(msg);
        if (!result) {
            logstream(LOG_INFO) << "failed to send msg to ["
                                 << addr << ":" << channels[0].second << "] "
                                 << strerror(errno) << LOG_endl;
        }

        pthread_spin_unlock(&send_locks[0]);

        return result;
    }
};

} // namespace ubiquant