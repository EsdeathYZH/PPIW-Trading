#include "order_sender.h"

#include "common/monitor.hpp"
#include "trader_controller.h"

namespace ubiquant {

TraderOrderSender::TraderOrderSender(int exchange_idx)
    : exchange_idx_(exchange_idx),
      order_queue_(Config::sliding_window_size * Config::stock_num / Config::exchange_num) {

    pthread_spin_init(&send_lock, 0);

    // init msg sender
    std::vector<std::pair<int, int>> port_pairs;
    auto& channels = Config::trader_port2exchange_port[Config::partition_idx][exchange_idx_];
    // we use the first two channels
    port_pairs.push_back(channels[0]);
    port_pairs.push_back(channels[1]);
    msg_sender_ = std::make_shared<MessageSender>(Config::exchanges_addr[exchange_idx_], port_pairs);
}

void TraderOrderSender::stop() {
    pthread_spin_lock(&send_lock);
}

void TraderOrderSender::restart() {
    pthread_spin_unlock(&send_lock);
}

void TraderOrderSender::reset_network() {
    // reset msg sender
    msg_sender_.reset();
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

    std::string order_msg;
    // order_msg.reserve((Config::sliding_window_size * Config::stock_num / Config::exchange_num+7) * sizeof(Order) + 2 * sizeof(uint32_t));
    while (true) {
        order_msg.clear();
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

        bool res = false;
        while (!res) {
            pthread_spin_lock(&send_lock);
            res = msg_sender_->send(order_msg);
            pthread_spin_unlock(&send_lock);
            if(unlikely(!res)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    }

    // monitor.end_thpt();
}

void TraderOrderSender::put_order(Order& order) {
    order_queue_.put(order);
}

}  // namespace ubiquant
