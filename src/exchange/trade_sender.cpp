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

CommTrade convert_trade_to_commtrade(const Trade& trade) {
    return CommTrade {
        stk_code : trade.stk_code,
        bid_id : trade.bid_id,
        ask_id : trade.ask_id,
        dummy : 0,
        price : trade.price,
        volume : trade.volume
    };
}

void ExchangeTradeSender::put_trade(Trade& trade) {
    // build trade msg
    std::string trade_msg;
    uint32_t msg_code = MSG_TYPE::TRADE_MSG;
    uint32_t cnt = 1;
    CommTrade commTrade = convert_trade_to_commtrade(trade);
    trade_msg.append((char*)&msg_code, sizeof(uint32_t));
    trade_msg.append((char*)&cnt, sizeof(uint32_t));
    trade_msg.append((char*)&commTrade, sizeof(commTrade));

    std::cout << "before serial trade:" << trade_msg.size() << std::endl;
    trade.print();
    // std::cout << "after serial trade " << std::endl;

    // TODO!: debug
    // std::vector<Trade> out_trades;
    // size_t out_offset = sizeof(uint32_t); // skip msg code
    // uint32_t out_cnt = 0;
    // get_elem_from_buf(trade_msg.c_str(), out_offset, out_cnt);
    // out_trades.resize(out_cnt);
    // std::cout << "out cnt=" << out_cnt << std::endl;
    // for(auto& trade : out_trades) {
    //     get_elem_from_buf(trade_msg.c_str(), out_offset, trade);
    //     trade.print();
    // }

    msg_queue_.put(trade_msg);
}

void ExchangeTradeSender::put_order_ack(OrderAck& ack) {
    // build ack msg
    std::string ack_msg;
    uint32_t msg_code = MSG_TYPE::ORDER_ACK_MSG;
    uint32_t cnt = 1;
    ack_msg.append((char*)&msg_code, sizeof(uint32_t));
    ack_msg.append((char*)&cnt, sizeof(uint32_t));
    ack_msg.append((char*)&ack, sizeof(ack));
    msg_queue_.put(ack_msg);
}

}  // namespace ubiquant

