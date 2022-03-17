#include "order_receiver.h"

#include "common/monitor.hpp"
#include "exchange.h"

namespace ubiquant {

ExchangeOrderReceiver::ExchangeOrderReceiver() {
    // init msg receivers
    std::vector<int> ports;
    for (int i = 0; i < Config::trader_num; i++) {
        ports.push_back(Config::trader_port2exchange_port[i][Config::partition_idx][0].second);
        ports.push_back(Config::trader_port2exchange_port[i][Config::partition_idx][1].second);
    }
    msg_receiver_ = std::make_shared<MessageReceiver>(ports);
}

void ExchangeOrderReceiver::run() {
    logstream(LOG_EMPH) << "Exchange OrderReceiver is running..." << LOG_endl;
    Monitor monitor;
    monitor.start_thpt();
    while (true) {
        std::string msg = msg_receiver_->recv();
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
