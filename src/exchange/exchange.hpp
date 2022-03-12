#pragma once

#include "common/config.hpp"
#include "common/thread.h"
#include "common/block_queue.hpp"
#include "common/sliding_window.hpp"
#include "trade_sender.h"
#include "stock_exchange.hpp"
#include "debug.hpp"

namespace ubiquant {

class Exchange {
private:
    std::vector<StockExchange> stock_exchange; /* [0] is not used */
    std::vector<SlidingWindow<Order>> order_buffer;

public:
    Exchange() {
        for (int i = 0; i <= Config::stock_num; ++i) {
            stock_exchange.emplace_back(i);
            order_buffer.emplace_back(Config::sliding_window_size);
        }
    };

    ~Exchange() {
        for (auto& exchange : stock_exchange) {
            exchange.join();
        }
    }

    void start() {
        for (auto& exchange : stock_exchange) {
            exchange.start();
        }
    }

    // Order receiver will call this function
    void receiveOrder(Order& order) {
        int window_idx = order.order_id % Config::sliding_window_size;
        order_buffer[order.stk_code].put(order, window_idx);
    }

    // Stock exchange will call this function
    std::vector<Order> comsumeOrder(int stk_code) {
        std::vector<Order> orders;
        Order order;
        while(order_buffer[stk_code].poll(order)) {
            orders.push_back(order);
        }
        // generate order ack and push into msg queue
        if(!orders.empty()) {
            OrderAck ack;
            ack.order_id = orders.back().order_id;
            ack.stk_code = stk_code;
            Global<ExchangeTradeSender>::Get()->put_order_ack(ack)
        }
        return orders;
    }

    // Stock exchange will call this function
    void produceTrade(Trade& new_trade) {
        Global<ExchangeTradeSender>::Get()->put_trade(new_trade);
    }

    StockExchange& getStockExchange(int stk_code) {
        return stock_exchange[stk_code];
    }

    /* For local processing */
    int handleSingleOrder(Order& order) {
        /* sanity check */
        assert(order.stk_code >= 1 && order.stk_code <= 10);

        order.print();
        int ret = stock_exchange[order.stk_code].receiveOrder(order);
        // if (ret != 0) {
        //     log("error number: %d\n", ret);
        // }

        return 0;

    }
};

}  // namespace ubiquant