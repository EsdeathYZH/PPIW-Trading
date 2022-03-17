#pragma once

#include <fcntl.h>
#include <unistd.h>

#include <memory>

#include "common/monitor.hpp"
#include "common/thread.h"
#include "common/type.hpp"
#include "network/msg_receiver.h"

namespace ubiquant {

class TraderTradeReceiver : public ubi_thread {
   public:
    TraderTradeReceiver();
    ~TraderTradeReceiver();

    void run() override;

   protected:
    constexpr static int EMPTY_FD = -1;
    constexpr static size_t FILE_TRUNC_SIZE = 1ul << 30;  // 1GB
    constexpr static size_t TRADE_BUF_THRESHOLD = 100;    // 1GB

    void process_trade_result(std::string& msg);
    void process_order_ack(std::string& msg);
    void flush();

    // socket server
    std::shared_ptr<MessageReceiver> msg_receiver_;

    std::unordered_map<int, int> trade_fds_;
    std::unordered_map<int, std::vector<Trade>> trade_buffer_;
    std::unordered_map<int, int> trade_idxs_;

    Monitor monitor;
};

}  // namespace ubiquant
