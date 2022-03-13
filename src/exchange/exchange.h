#pragma once

#include "common/config.hpp"
#include "common/thread.h"
#include "common/block_queue.hpp"
#include "common/sliding_window.hpp"
#include "trade_sender.h"
#include "stock_exchange.h"
#include "debug.hpp"

namespace ubiquant {

class StockExchange;

class Exchange {
private:
    std::unordered_map<int, std::shared_ptr<StockExchange>> stock_exchange; /* [0] is not used */
    std::unordered_map<int, SlidingWindow<Order>> order_buffer;

public:
    Exchange();

    ~Exchange();

    void Run();

    // Order receiver will call this function
    void receiveOrder(Order& order);

    // Stock exchange will call this function
    std::vector<Order> comsumeOrder(int stk_code);

    // Stock exchange will call this function
    void produceTrade(Trade& new_trade);

    inline std::shared_ptr<StockExchange> getStockExchange(int stk_code) {
        return stock_exchange[stk_code];
    }

    /* For local processing */
    int handleSingleOrder(Order& order);
};

}  // namespace ubiquant