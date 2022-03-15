#include "order_sender.h"

namespace ubiquant {

TraderOrderSender::TraderOrderSender(int exchange_idx) 
    : exchange_idx_(exchange_idx), 
      order_queue_(Config::sliding_window_size*Config::stock_num/Config::exchange_num) {
    
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
    while(true) {
        std::string order_msg;
        uint32_t msg_code = MSG_TYPE::ORDER_MSG;
        uint32_t cnt = 0;
        order_msg.append((char*)&msg_code, sizeof(uint32_t));
        order_msg.append((char*)&cnt, sizeof(uint32_t));
        Order order;
        while(order_queue_.poll(order)) {
            order.append_to_str(order_msg);
            cnt++;
            if(cnt >= Config::sliding_window_size) break;
        }
        *((uint32_t*)(order_msg.data()+sizeof(uint32_t))) = cnt;
        if(!msg_sender_->send(order_msg)) {
            logstream(LOG_ERROR) << "Order sending error!" << LOG_endl;
            ASSERT(false);
        }
    }
}

void TraderOrderSender::put_order(Order& order) {
    order_queue_.put(order);
}

}  // namespace ubiquant

