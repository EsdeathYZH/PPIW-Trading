#include "order_sender.h"

namespace ubiquant {

TraderOrderSender::TraderOrderSender() {
    
}

void TraderOrderSender::run() {
    while(true) {
        auto order = order_queue_.take();
        // TODO: build order message
        std::string order_msg;
        if(!msg_sender_->send(order_msg)) {
            logstream(LOG_ERROR) << "Order sending error!" << LOG_endl;
        }
    }
}

void TraderOrderSender::put_order(Order& order) {
    order_queue_.put(order);
}

}  // namespace ubiquant

