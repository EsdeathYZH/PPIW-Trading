#pragma once

#include "stock_exchange.hpp"
#include "debug.hpp"

class Exchange {
private:
    std::vector<StockExchange> stock_exchange; /* [0] is not used */
public:
    Exchange() {
        for (int i = 0; i <= 10; ++i) {
            stock_exchange.emplace_back(i);
        }
    };

    int handleSingleOrder(Order& order) {
        /* sanity check */
        assert(order.order_id >= 1 && order.order_id <= 10);

        int ret = stock_exchange[order.order_id].receiveOrder(order);
        if (ret != 0) {
            log("error number: %d\n", ret);
        }

        return -1;

    }
};