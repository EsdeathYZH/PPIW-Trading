#include "trade_receiver.h"

namespace ubiquant {

TraderTradeReceiver::TraderTradeReceiver(std::vector<int> stk_codes) {
    ASSERT(!Config::data_folder.empty());
    for(auto code : stk_codes) {
        std::string path = Config::data_folder + "/trade_res." + std::to_string(code);
        trade_fds_[code] = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0640);
        if (trade_fds_[code] == EMPTY_FD)
            throw std::runtime_error("open wal file error.");
        if (ftruncate(trade_fds_[code], FILE_TRUNC_SIZE) != 0)
            throw std::runtime_error("ftruncate wal file error.");

        trade_buffer_[code] = std::vector<Trade>();
        trade_buffer_[code].reserve(TRADE_BUF_THRESHOLD);
    }
}

TraderTradeReceiver::~TraderTradeReceiver()
{
    // flush all trades
    flush();
    // close fd
    for(auto [stk_code, fd] : trade_fds_) {
        if (fd != EMPTY_FD) close(fd);
    }
}

void TraderTradeReceiver::run() {
    while(true) {
        std::string msg = msg_receiver_->recv();
        // TODO: process OrderAck or Trade
    }
}

void TraderTradeReceiver::process_trade_result(std::string& msg) {
    // de-serialize
    std::vector<Trade> trades;
    for(auto& trade : trades) {
        trade_buffer_[trade.stk_code].push_back(trade);
        // batch write
        if(trade_buffer_[trade.stk_code].size() >= TRADE_BUF_THRESHOLD) {
            if ((size_t)write(trade_fds_[trade.stk_code], 
                              trade_buffer_[trade.stk_code].data(), 
                              trade_buffer_[trade.stk_code].size() * sizeof(Trade)) 
                    != trade_buffer_[trade.stk_code].size() * sizeof(Trade)) {
                throw std::runtime_error("write trade file error.");
            }

            if (fdatasync(trade_fds_[trade.stk_code]) != 0)
                throw std::runtime_error("fdatasync trade file error.");

            trade_buffer_[trade.stk_code].clear();
        }
    }
}

void TraderTradeReceiver::flush() {
    for(auto& [stk_code, buffer] : trade_buffer_) {
        // batch write
        if ((size_t)write(trade_fds_[stk_code], 
                          buffer.data(), 
                          buffer.size() * sizeof(Trade)) 
                != buffer.size() * sizeof(Trade)) {
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

    // TODO: pass the info to controller
}

}  // namespace ubiquant

