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

class MessageReceiver {
private:
    zmq::context_t context;
    std::vector<int> ports;
    std::unordered_map<int, zmq::socket_t*> receivers;     // static allocation

public:
    MessageReceiver(std::vector<int> receiver_ports)
        : context(1), ports(receiver_ports) {

        for (auto port : receiver_ports) {
            receivers[port] = new zmq::socket_t(context, ZMQ_PULL);
            char address[32] = "";
            snprintf(address, 32, "tcp://*:%d", port);
            receivers[port]->bind(address);
            if(errno) std::cout << "Errno:" << errno << std::endl;
            std::cout << "Bind on port:" << port << std::endl;
        }
    }

    ~MessageReceiver() {
        for (auto &r : receivers)
            if (r.second) delete r.second;
    }

    std::string recv() {
        std::string msg;
        int idx = 0;
        // poll all recv ports
        while(true) {
            if (tryrecv(idx, msg)) {
                return msg;
            }
            idx = (idx + 1) % ports.size();
        }
    }


    bool tryrecv(int idx, std::string &str) {
        zmq::message_t msg;
        bool success = false;

        if (success = receivers[ports[idx]]->recv(&msg, ZMQ_NOBLOCK))
            str = std::string((char *)msg.data(), msg.size());

        return success;
    }

};

} // namespace ubiquant