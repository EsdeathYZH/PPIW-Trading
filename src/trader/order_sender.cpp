#include "order_sender.h"

namespace ubiquant {

TraderOrderSender::TraderOrderSender() 
    : order_queue_(Config::sliding_window_size*Config::stock_num) {

}

void TraderOrderSender::run() {
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

