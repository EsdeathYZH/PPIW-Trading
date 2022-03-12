#include "order_receiver.h"
#include "exchange.h"

namespace ubiquant {

ExchangeOrderReceiver::ExchangeOrderReceiver() {
    
}

void ExchangeOrderReceiver::run() {
    while(true) {
        std::string msg = msg_receiver_->recv();
        // deserialize orders
        size_t offset = 0;
        uint32_t msg_code = 0;
        get_elem_from_buf(msg.c_str(), offset, msg_code);
        ASSERT_MSG(msg_code == MSG_TYPE::ORDER_MSG, "Wrong message type!");
        uint32_t cnt = 0;
        get_elem_from_buf(msg.c_str(), offset, cnt);
        Order order;
        for(uint32_t i = 0; i < cnt; i++) {
            order.from_str(msg, offset);
            Global<Exchange>::Get()->receiveOrder(order);
        }
    }
}

}  // namespace ubiquant

