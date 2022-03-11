#include "trade_sender.h"

namespace ubiquant {

ExchangeTradeSender::ExchangeTradeSender() {
    
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

}  // namespace ubiquant

