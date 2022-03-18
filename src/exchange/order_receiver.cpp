#include "order_receiver.h"

#include "common/monitor.hpp"
#include "exchange.h"

namespace ubiquant {

ExchangeOrderReceiver::ExchangeOrderReceiver() {

    pthread_spin_init(&recv_lock, 0);

    // init msg receivers
    std::vector<int> ports;
    for (int i = 0; i < Config::trader_num; i++) {
        ports.push_back(Config::trader_port2exchange_port[i][Config::partition_idx][0].second);
        ports.push_back(Config::trader_port2exchange_port[i][Config::partition_idx][1].second);
    }
    msg_receiver_ = std::make_shared<MessageReceiver>(ports);
}

void ExchangeOrderReceiver::stop() {
    std::cout << "ExchangeOrderReceiver Lock recv lock" << std::endl;
    pthread_spin_lock(&recv_lock);
    std::cout << "ExchangeOrderReceiver Lock recv lock success" << std::endl;
}

void ExchangeOrderReceiver::restart() {
    std::cout << "ExchangeOrderReceiver unlock recv lock" << std::endl;
    pthread_spin_unlock(&recv_lock);
    std::cout << "ExchangeOrderReceiver unlock recv lock success" << std::endl;
}

void ExchangeOrderReceiver::reset_network() {
    // reset msg receivers
    std::vector<int> ports;
    for (int i = 0; i < Config::trader_num; i++) {
        ports.push_back(Config::trader_port2exchange_port[i][Config::partition_idx][0].second);
        ports.push_back(Config::trader_port2exchange_port[i][Config::partition_idx][1].second);
    }
    msg_receiver_->reset_port(ports);
}

void ExchangeOrderReceiver::run() {
    logstream(LOG_EMPH) << "Exchange OrderReceiver is running..." << LOG_endl;
    Monitor monitor;
    monitor.start_thpt();
    while (true) {
        std::string msg;
        bool res = false;
        while (!res) {
            pthread_spin_lock(&recv_lock);
            res = msg_receiver_->tryrecv(msg);
            pthread_spin_unlock(&recv_lock);
            // if(unlikely(!res)) {
            //     std::this_thread::sleep_for(std::chrono::milliseconds(100));
            // }
        }

        // deserialize orders
        size_t offset = 0;
        uint32_t msg_code = 0;
        get_elem_from_buf(msg.c_str(), offset, msg_code);
        ASSERT_MSG(msg_code == MSG_TYPE::ORDER_MSG, "Wrong message type!");
        uint32_t cnt = 0;
        get_elem_from_buf(msg.c_str(), offset, cnt);
        Order order;
        for (uint32_t i = 0; i < cnt; i++) {
            get_elem_from_buf(msg.c_str(), offset, order);
            Global<Exchange>::Get()->receiveOrder(order);
            monitor.add_cnt();
        }
        monitor.print_timely_thpt("Order Receiver Throughput");
    }
    monitor.end_thpt();
}

}  // namespace ubiquant
