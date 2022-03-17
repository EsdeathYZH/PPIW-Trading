#include "order_sender.h"

#include "common/monitor.hpp"
#include "trader_controller.h"

namespace ubiquant {

TraderOrderSender::TraderOrderSender(int exchange_idx)
    : exchange_idx_(exchange_idx),
      order_queue_(Config::sliding_window_size * Config::stock_num / Config::exchange_num) {
    // init msg sender
    std::vector<std::pair<int, int>> port_pairs;
    auto& channels = Config::trader_port2exchange_port[Config::partition_idx][exchange_idx_];
    // we use the first two channels
    port_pairs.push_back(channels[0]);
    port_pairs.push_back(channels[1]);
    msg_sender_ = std::make_shared<MessageSender>(Config::exchanges_addr[exchange_idx_], port_pairs);
}

void TraderOrderSender::run() {
    logstream(LOG_EMPH) << "Trader OrderSender is running..." << LOG_endl;
    while (!Global<TraderController>::Get() || !Global<TraderController>::Get()->is_inited()) {
        usleep(1);
    }

    // Monitor monitor;
    // monitor.start_thpt();

    while (true) {
        std::string order_msg;
        uint32_t msg_code = MSG_TYPE::ORDER_MSG;
        uint32_t cnt = 0;
        order_msg.append((char*)&msg_code, sizeof(uint32_t));
        order_msg.append((char*)&cnt, sizeof(uint32_t));
        Order order;
        while (!cnt) {
            if (!order_queue_.poll(order)) continue;
            // std::cout << "Send a order:" << std::endl;
            // order.print();
            order_msg.append((char*)&order, sizeof(order));
            cnt++;
            // monitor.add_cnt();
            // monitor.print_timely_thpt("Order Sender Throughput");
            if (cnt >= 100) break;
        }
        *((uint32_t*)(order_msg.data() + sizeof(uint32_t))) = cnt;
        while (!msg_sender_->send(order_msg)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    // monitor.end_thpt();
}

void TraderOrderSender::put_order(Order& order) {
    order_queue_.put(order);
}

}  // namespace ubiquant
