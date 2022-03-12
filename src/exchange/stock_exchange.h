#pragma once

#include "common/type.hpp"
#include "common/global.hpp"
#include "stock_decl_book.hpp"
#include "record.hpp"
#include "debug.hpp"
#include "exchange.h"

namespace ubiquant {

class StockExchange : public ubi_thread {
/**
 * @brief 单只股票的 exchange 处理
 */

private:
    /* 股票编号 */
    int stk_code;
    /* 集中申报簿 */
    StockDeclarationBook decl_book;
    /* 尚未轮到的 order */
    std::vector<Order> not_ready_orders;
    /* 最后成功 commit 的 order_id */
    int last_commit_order_id;

    /* 输出：Trade 的序列 */
    std::vector<Trade> trade_list;

    /* helpers */
    static bool orderGtById(Order& o1, Order& o2) {
        return o1.order_id > o2.order_id;
    };

public:
    StockExchange(int stk_code);

    void run() override;

    std::vector<Order> comsumeOrder();

    void produceTrade(Trade& new_trade);

    inline std::vector<Trade>& getTradeList() {
        return trade_list;
    }

    int receiveOrder(Order& order);

    int commitOrder(Order& order);
};

}  // namespace ubiquant