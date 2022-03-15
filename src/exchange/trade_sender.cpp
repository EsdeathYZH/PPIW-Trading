#include "trade_sender.h"

namespace ubiquant {

ExchangeTradeSender::ExchangeTradeSender() 
    : msg_queue_(Config::sliding_window_size * Config::stock_num / 2){
    // init msg senders
    for(int i = 0; i < Config::trader_num; i++) {
        std::vector<std::pair<int, int>> port_pairs;
        auto& channels = Config::trader_port2exchange_port[i][Config::partition_idx];
        port_pairs.push_back({channels[2].second, channels[2].first});
        msg_senders_.push_back(std::make_shared<MessageSender>(Config::traders_addr[i], port_pairs));
    }
}

void ExchangeTradeSender::run() {
    logstream(LOG_EMPH) << "Exchange TradeSender is running..." << LOG_endl;
    while(true) {
        auto msg = msg_queue_.take();
        for(auto& sender : msg_senders_) {
            if(!sender->send(msg)) {
                logstream(LOG_ERROR) << "Message sending error!" << LOG_endl;
            }
        }
    }
}

void ExchangeTradeSender::put_trade(Trade& trade) {
    // build trade msg
    std::string trade_msg;
    uint32_t msg_code = MSG_TYPE::TRADE_MSG;
    uint32_t cnt = 1;
    trade_msg.append((char*)&msg_code, sizeof(uint32_t));
    trade_msg.append((char*)&cnt, sizeof(uint32_t));
    trade.append_to_str(trade_msg);
    msg_queue_.put(trade_msg);
}

void ExchangeTradeSender::put_order_ack(OrderAck& ack) {
    // build ack msg
    std::string ack_msg;
    uint32_t msg_code = MSG_TYPE::ORDER_ACK_MSG;
    uint32_t cnt = 1;
    ack_msg.append((char*)&msg_code, sizeof(uint32_t));
    ack_msg.append((char*)&cnt, sizeof(uint32_t));
    ack.append_to_str(ack_msg);
    msg_queue_.put(ack_msg);
}

}  // namespace ubiquant

