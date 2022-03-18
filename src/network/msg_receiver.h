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
#include <thread>


#include "common/config.h"

// utils
#include "utils/logger2.hpp"
#include "utils/assertion.hpp"

namespace ubiquant {

class MessageReceiver {
private:
    zmq::context_t context;
    std::vector<int> ports;
    std::vector<zmq::socket_t*> receivers;     // static allocation

    int offset = 0;

public:
    MessageReceiver(std::vector<int> receiver_ports)
        : context(1), ports(receiver_ports) {

        for (auto port : receiver_ports) {
            auto socket = new zmq::socket_t(context, ZMQ_PULL);
            char address[32] = "";
            snprintf(address, 32, "tcp://*:%d", port);
            socket->bind(address);
            std::cout << "Bind on address:" << address << std::endl;
            receivers.push_back(socket);
        }
    }

    ~MessageReceiver() {
        for(int idx = 0; idx < receivers.size(); idx++) {
            auto port = ports[idx];
            auto& socket = receivers[idx];
            if (socket) {
                char address[32] = "";
                snprintf(address, 32, "tcp://*:%d", port);
                socket->unbind(address);
                std::cout << "Unbind from address:" << address << std::endl;
                delete socket;
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    void reset_port(std::vector<int> new_ports) {
        assert(new_ports.size() == ports.size());
        for(int idx = 0; idx < ports.size(); idx++) {
            int port = ports[idx];
            auto& socket = receivers[idx];
            if (socket) {
                char address[32] = "";
                snprintf(address, 32, "tcp://*:%d", port);
                socket->unbind(address);
                std::cout << "Unbind from address:" << address << std::endl;
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));

        for(int idx = 0; idx < new_ports.size(); idx++) {
            int new_port = new_ports[idx];
            auto& socket = receivers[idx];
            if (socket) {
                char address[32] = "";
                snprintf(address, 32, "tcp://*:%d", new_port);
                socket->bind(address);
                std::cout << "Bind on address:" << address << std::endl;
            }
        }

        ports.swap(new_ports);
    }

    std::string recv() {
        std::string msg;
        // poll all recv ports
        while(true) {
            if (tryrecv(offset, msg)) {
                return msg;
            }
            offset = (offset + 1) % ports.size();
        }
    }

    bool tryrecv(std::string &str) {
        // poll all recv ports
        for(int idx = 0; idx < ports.size(); idx++) {
            if (tryrecv((idx+offset) % ports.size(), str)) {
                return true;
            }
        }
        offset = (offset + 1) % ports.size();
        return false;
    }


    bool tryrecv(int idx, std::string &str) {
        zmq::message_t msg;
        bool success = false;

        if (success = receivers[idx]->recv(&msg, ZMQ_NOBLOCK))
            str = std::string((char *)msg.data(), msg.size());

        return success;
    }

};

} // namespace ubiquant