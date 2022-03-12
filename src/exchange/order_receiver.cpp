#include "order_receiver.h"
#include "exchange.h"

namespace ubiquant {

ExchangeOrderReceiver::ExchangeOrderReceiver() {
    
}

void ExchangeOrderReceiver::run() {
    while(true) {
        std::string msg = msg_receiver_->recv();
        // TODO: deserialize Order
        Order order;
        Global<Exchange>::Get()->receiveOrder(order);
    }
}

}  // namespace ubiquant

