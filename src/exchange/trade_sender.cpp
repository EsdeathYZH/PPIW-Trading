#include "trade_sender.h"

namespace ubiquant {

ExchangeTradeSender::ExchangeTradeSender() 
    : msg_queue_(Config::sliding_window_size * Config::stock_num / 2){
    
}

void ExchangeTradeSender::run() {
    while(true) {
        auto msg = msg_queue_.take();
        if(!msg_sender_->send(msg)) {
            logstream(LOG_ERROR) << "Message sending error!" << LOG_endl;
        }
    }
}

void ExchangeTradeSender::put_trade(Trade& trade) {
    // TODO: build trade msg
    std::string trade_msg;
    msg_queue_.put(trade_msg);
}

void ExchangeTradeSender::put_order_ack(OrderAck& ack) {
    // TODO: build ack msg
    std::string ack_msg;
    msg_queue_.put(ack_msg);
}

}  // namespace ubiquant

