#include "trade_receiver.h"

#include "common/global.hpp"
#include "common/monitor.hpp"
#include "trader_controller.h"

namespace ubiquant {

TraderTradeReceiver::TraderTradeReceiver() {

    pthread_spin_init(&recv_lock, 0);

    // init stock code (ALL)
    std::vector<int> stk_codes;
    // NOTICE: stock code starts from 1
    for (int i = 1; i <= Config::stock_num; i++) {
        stk_codes.push_back(i);
    }

    ASSERT(!Config::trade_output_folder.empty());
    for (auto code : stk_codes) {
        std::string path = Config::trade_output_folder + "/trade_res." + std::to_string(code);
        trade_fds_[code] = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0640);
        if (trade_fds_[code] == EMPTY_FD)
            throw std::runtime_error("open wal file error.");
        // if (ftruncate(trade_fds_[code], FILE_TRUNC_SIZE) != 0)
        //     throw std::runtime_error("ftruncate wal file error.");

        trade_buffer_[code] = std::vector<Trade>();
        trade_buffer_[code].reserve(TRADE_BUF_THRESHOLD);

        // NOTICE: trade_idx starts from 1
        trade_idxs_[code] = 1;
    }

    // init msg receivers
    std::vector<int> ports;
    for (int i = 0; i < Config::exchange_num; i++) {
        ports.push_back(Config::trader_port2exchange_port[Config::partition_idx][i][2].first);
    }
    msg_receiver_ = std::make_shared<MessageReceiver>(ports);
}

void TraderTradeReceiver::stop() {
    std::cout << "TraderTradeReceiver Lock recv lock" << std::endl;
    pthread_spin_lock(&recv_lock);
    std::cout << "TraderTradeReceiver Lock recv lock success" << std::endl;
}

void TraderTradeReceiver::restart() {
    std::cout << "TraderTradeReceiver unlock recv lock" << std::endl;
    pthread_spin_unlock(&recv_lock);
    std::cout << "TraderTradeReceiver unlock recv lock success" << std::endl;
}

void TraderTradeReceiver::reset_network() {
    // // reset msg receiver
    msg_receiver_.reset();
    std::vector<int> ports;
    for (int i = 0; i < Config::exchange_num; i++) {
        ports.push_back(Config::trader_port2exchange_port[Config::partition_idx][i][2].first);
    }
    msg_receiver_ = std::make_shared<MessageReceiver>(ports);
}

TraderTradeReceiver::~TraderTradeReceiver() {
    // flush all trades
    flush();
    // close fd
    for (auto [stk_code, fd] : trade_fds_) {
        if (fd != EMPTY_FD) close(fd);
    }
}

void TraderTradeReceiver::run() {
    while (!Global<TraderController>::Get() || !Global<TraderController>::Get()->is_inited()) {
        usleep(1);
    }
    logstream(LOG_EMPH) << "Trader TradeReceiver is running..." << LOG_endl;

    monitor.start_thpt();
    while (true) {
        std::string msg;
        bool res = false;
        while (!res) {
            pthread_spin_lock(&recv_lock);
            res = msg_receiver_->tryrecv(msg);
            pthread_spin_unlock(&recv_lock);
            // if(unlikely(!res)) {
            //     std::this_thread::sleep_for(std::chrono::milliseconds(100));
            // }
        }

        uint32_t msg_code;
        size_t offset = 0;
        get_elem_from_buf(msg.c_str(), offset, msg_code);
        if (msg_code == MSG_TYPE::ORDER_ACK_MSG) {
            process_order_ack(msg);
        } else if (msg_code == MSG_TYPE::TRADE_MSG) {
            process_trade_result(msg);
        } else {
            ASSERT_MSG(false, "Wrong message code!");
        }
        monitor.print_timely_thpt("Trade Receiver Throughput");
    }
    monitor.end_thpt();
}

Trade convert_commtrade_to_trade(const CommTrade& commTrade) {
    return Trade{
        stk_code : commTrade.stk_code,
        bid_id : commTrade.bid_id,
        ask_id : commTrade.ask_id,
        price : commTrade.price,
        volume : commTrade.volume
    };
}

void TraderTradeReceiver::process_trade_result(std::string& msg) {
    // de-serialize
    std::vector<Trade> trades;
    size_t offset = sizeof(uint32_t);  // skip msg code
    uint32_t cnt = 0;
    get_elem_from_buf(msg.c_str(), offset, cnt);
    trades.resize(cnt);
    for (auto& trade : trades) {
        CommTrade commTrade;
        get_elem_from_buf(msg.c_str(), offset, commTrade);
        trade = convert_commtrade_to_trade(commTrade);
        monitor.add_cnt();
    }

    for (auto& trade : trades) {
        // trade.print();
        // update hook in controller
        Global<TraderController>::Get()->update_if_hooked(trade.stk_code, trade_idxs_[trade.stk_code], trade.volume);
        trade_idxs_[trade.stk_code]++;

        trade_buffer_[trade.stk_code].push_back(trade);
        // batch write
        if (trade_buffer_[trade.stk_code].size() >= TRADE_BUF_THRESHOLD) {
            if ((size_t)write(trade_fds_[trade.stk_code],
                              trade_buffer_[trade.stk_code].data(),
                              trade_buffer_[trade.stk_code].size() * sizeof(Trade)) != trade_buffer_[trade.stk_code].size() * sizeof(Trade)) {
                throw std::runtime_error("write trade file error.");
            }

            if (fdatasync(trade_fds_[trade.stk_code]) != 0)
                throw std::runtime_error("fdatasync trade file error.");

            trade_buffer_[trade.stk_code].clear();
            trade_buffer_[trade.stk_code].shrink_to_fit();
            trade_buffer_.reserve(TRADE_BUF_THRESHOLD);
        }
    }
}

void TraderTradeReceiver::flush() {
    for (auto& [stk_code, buffer] : trade_buffer_) {
        // batch write
        if ((size_t)write(trade_fds_[stk_code],
                          buffer.data(),
                          buffer.size() * sizeof(Trade)) != buffer.size() * sizeof(Trade)) {
            throw std::runtime_error("write trade file error.");
        }

        if (fdatasync(trade_fds_[stk_code]) != 0)
            throw std::runtime_error("fdatasync trade file error.");

        buffer.clear();
    }
}

void TraderTradeReceiver::process_order_ack(std::string& msg) {
    // de-serialize
    std::vector<OrderAck> acks;
    size_t offset = sizeof(uint32_t);  // skip msg code
    uint32_t cnt = 0;
    get_elem_from_buf(msg.c_str(), offset, cnt);
    acks.resize(cnt);
    for (auto& ack : acks) {
        get_elem_from_buf(msg.c_str(), offset, ack);
    }

    // update sliding window start in controller
    for (auto& ack : acks) {
        Global<TraderController>::Get()->update_sliding_window_start(ack.stk_code, ack.order_id + 1);
    }
}

}  // namespace ubiquant
