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

    StockExchange& getStockExchange(int stk_code) {
        return stock_exchange[stk_code];
    }

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