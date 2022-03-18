#include "trade_sender.h"

#include "common/monitor.hpp"

namespace ubiquant {

ExchangeTradeSender::ExchangeTradeSender()
    : msg_queue_(Config::sliding_window_size * Config::stock_num / 2) {

    pthread_spin_init(&send_lock, 0);

    // init msg senders
    for (int i = 0; i < Config::trader_num; i++) {
        std::pair<int, int> port_pair;
        auto& channels = Config::trader_port2exchange_port[i][Config::partition_idx];
        port_pair = {channels[2].second, channels[2].first};
        msg_senders_.push_back(std::make_shared<MessageSender>(
            Config::exchanges_addr[Config::partition_idx], 
            Config::traders_addr[i], 
            port_pair));
    }
}

void ExchangeTradeSender::stop() {
    std::cout << "ExchangeTradeSender Lock send lock" << std::endl;
    pthread_spin_lock(&send_lock);
    std::cout << "ExchangeTradeSender Lock send lock success" << std::endl;
}

void ExchangeTradeSender::restart() {
    std::cout << "ExchangeTradeSender Unlock send lock" << std::endl;
    pthread_spin_unlock(&send_lock);
    std::cout << "ExchangeTradeSender Unlock send lock success" << std::endl;
}

void ExchangeTradeSender::reset_network() {
    // reset msg senders
    for (int i = 0; i < Config::trader_num; i++) {
        std::pair<int, int> port_pair;
        auto& channels = Config::trader_port2exchange_port[i][Config::partition_idx];
        port_pair = {channels[2].second, channels[2].first};
        msg_senders_[i]->reset_port(port_pair);
    }
    after_reset = true;
}

void ExchangeTradeSender::run() {
    logstream(LOG_EMPH) << "Exchange TradeSender is running..." << LOG_endl;
    // Monitor monitor;
    // monitor.start_thpt();
    while (true) {
        auto msg = msg_queue_.take();
        for (int idx = 0; idx < msg_senders_.size(); idx++) {
            bool res = false;
            while (!res) {
                pthread_spin_lock(&send_lock);
                res = msg_senders_[idx]->send(msg);
                pthread_spin_unlock(&send_lock);
                // if(unlikely(!res)) {
                //     std::this_thread::sleep_for(std::chrono::milliseconds(100));
                // }
            }
        }

        // monitor.add_cnt();
        // monitor.print_timely_thpt("Trade Sender Throughput");
    }
    // monitor.end_thpt();
}

CommTrade convert_trade_to_commtrade(const Trade& trade) {
    return CommTrade{
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

    // std::cout << "before serial trade:" << trade_msg.size() << std::endl;
    // trade.print();
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
