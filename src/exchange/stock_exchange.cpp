#include "stock_exchange.h"

namespace ubiquant {

StockExchange::StockExchange(int stk_code) : stk_code(stk_code), last_commit_order_id(0) {}

void StockExchange::run() {
    logstream(LOG_EMPH) << "Exchange StockExchange [" << stk_code << "] is running..." << LOG_endl;
    while(true) {
        std::vector<Order> orders = comsumeOrder();
        for(auto& order : orders) {
            receiveOrder(order);
        }
    }
}

std::vector<Order> StockExchange::comsumeOrder() {
    return Global<Exchange>::Get()->comsumeOrder(stk_code);
}

void StockExchange::produceTrade(Trade& new_trade) {
    // version1: local vector storage
    // trade_list.push_back(new_trade);

    // version2: call Exchange to push into global msg queue
    Global<Exchange>::Get()->produceTrade(new_trade);
}

int StockExchange::receiveOrder(Order& order) {
    /* sanity check */
    assert(order.stk_code == stk_code);

    int ret = 0;

    /**
     * 1. push it into `not_ready_orders`
     * 2. pop and handle not_ready_orders until the head is not `last_commit_order_id + 1`
     */

    /* `not_ready_orders` should maintain as heap */
    /* TODO: should maintain as a sliding window */
    not_ready_orders.push_back(order);
    std::push_heap(not_ready_orders.begin(), not_ready_orders.end(), orderGtById);

    while (true) {
        if (not_ready_orders.empty())
            break;
        if (last_commit_order_id + 1 != not_ready_orders[0].order_id)
            break;

        /* Handle single order */
        ret = commitOrder(not_ready_orders[0]);
        if (ret != 0) {
            ex_debug("[%d] status ret=%d\n", last_commit_order_id + 1, ret);
        }

        /* Erase commited order */
        std::pop_heap(not_ready_orders.begin(), not_ready_orders.end(), orderGtById);
        not_ready_orders.pop_back();

        /* Increase last_commit_id */
        last_commit_order_id++;
    }

    return ret;
}

/* Type 0 */
int StockExchange::handleLimitOrder(Order& order)
{
    int left_volume = order.volume;
    if (order.direction == 1) {
        /* Buy in */
        /* ?????????????????????????????????????????????????????????????????????????????? */
        while (true) {
            if (left_volume == 0)
                break;
            SellRecord *sr = decl_book.querySellFirst();
            if (sr == nullptr)
                break;
            if (sr->price > order.price)
                break;

            if (left_volume < sr->volume) {
                Trade new_trade = {
                    stk_code:   stk_code,
                    bid_id:     order.order_id,
                    ask_id:     sr->order_id,
                    price:      sr->price,
                    volume:     left_volume,
                };
                produceTrade(new_trade);

                sr->volume -= left_volume;
                left_volume -= left_volume;
            } else {
                Trade new_trade = {
                    stk_code:   stk_code,
                    bid_id:     order.order_id,
                    ask_id:     sr->order_id,
                    price:      sr->price,
                    volume:     sr->volume,
                };
                produceTrade(new_trade);

                left_volume -= sr->volume,
                decl_book.removeSellFirst();
            }
        }

        if (left_volume != 0) {
            BuyRecord new_br = {
                order.order_id,
                order.price,
                left_volume
            };
            decl_book.insertBuyDecl(new_br);
        }
    } else if (order.direction == -1) {
        /* Sell out */
        /* ????????????????????????????????? */
        while (true) {
            if (left_volume == 0)
                break;
            BuyRecord *br = decl_book.queryBuyFirst();
            if (br == nullptr)
                break;
            if (br->price < order.price)
                break;

            if (left_volume < br->volume) {
                Trade new_trade = {
                    stk_code:   stk_code,
                    bid_id:     br->order_id,
                    ask_id:     order.order_id,
                    price:      br->price,
                    volume:     left_volume,
                };
                produceTrade(new_trade);

                br->volume -= left_volume;
                left_volume -= left_volume;
            } else {
                Trade new_trade = {
                    stk_code:   stk_code,
                    bid_id:     br->order_id,
                    ask_id:     order.order_id,
                    price:      br->price,
                    volume:     br->volume,
                };
                produceTrade(new_trade);

                left_volume -= br->volume;
                decl_book.removeBuyFirst();
            }
        }

        if (left_volume != 0) {
            SellRecord new_sr = {
                order.order_id,
                order.price,
                left_volume
            };
            decl_book.insertSellDecl(new_sr);
        }
    } else {
        ex_debug("strange order direction: %d\n", order.direction);
    }

    return 0;
}

/* Type 1 */
int StockExchange::handleCounterpartyBest(Order& order)
{
    /* ???????????????????????????????????????????????????????????????????????????????????????????????????*/
    if (order.direction == 1) {
        /* Buy in */
        SellRecord* sr = decl_book.querySellFirst();

        /* ??????????????????????????????????????????????????????????????????????????? */
        if (sr == nullptr)
            return -1;

        double t_price = sr->price;
        int left_volume = order.volume;

        /* NOTE: ??????????????????????????????????????? SellRecord */
        while (left_volume) {
            if (left_volume < sr->volume) {
                /* ??????????????????????????????????????????????????? */
                Trade new_trade = {
                    stk_code:   stk_code,
                    bid_id:     order.order_id,
                    ask_id:     sr->order_id,
                    price:      t_price,
                    volume:     left_volume,
                };
                produceTrade(new_trade);

                sr->volume -= left_volume;
                left_volume -= left_volume;
            } else {
                /* ????????????????????????????????????????????? */
                Trade new_trade = {
                    stk_code:   stk_code,
                    bid_id:     order.order_id,
                    ask_id:     sr->order_id,
                    price:      t_price,
                    volume:     sr->volume,
                };
                produceTrade(new_trade);

                left_volume -= sr->volume;
                decl_book.removeSellFirst();

                if (left_volume == 0)
                    break;

                sr = decl_book.querySellFirst();
                if (sr == nullptr || sr->price != t_price) {
                    /* ???????????????????????????????????????????????????????????????????????????????????? */
                    BuyRecord new_br = {
                        order.order_id,
                        t_price,
                        left_volume};
                    decl_book.insertBuyDecl(new_br);

                    left_volume -= left_volume;
                }
            }
        }
    } else if (order.direction == -1) {
        /* Sell out */
        BuyRecord* br = decl_book.queryBuyFirst();

        if (br == nullptr)
            return -1;

        double t_price = br->price;
        int left_volume = order.volume;

        while (left_volume) {
            if (left_volume < br->volume) {
                Trade new_trade = {
                    stk_code:   stk_code,
                    bid_id:     br->order_id,
                    ask_id:     order.order_id,
                    price:      t_price,
                    volume:     left_volume,
                };
                produceTrade(new_trade);

                br->volume -= left_volume;
                left_volume -= left_volume;
            } else {
                Trade new_trade = {
                    stk_code:    stk_code,
                    bid_id:      br->order_id,
                    ask_id:      order.order_id,
                    price:       t_price,
                    volume:      br->volume,
                };
                produceTrade(new_trade);

                left_volume -= br->volume;
                decl_book.removeBuyFirst();

                if (left_volume == 0)
                    break;

                br = decl_book.queryBuyFirst();
                if (br == nullptr || br->price != t_price) {
                    SellRecord new_sr = {
                        order.order_id,
                        t_price,
                        left_volume};
                    decl_book.insertSellDecl(new_sr);

                    left_volume -= left_volume;
                }
            }
        }
    } else {
        ex_debug("strange order direction: %d\n", order.direction);
    }
    return 0;
}

/* Type 2 */
int StockExchange::handleOurBest(Order& order)
{
    /* ????????????????????????????????????????????????????????????????????????????????????????????????*/
    if (order.direction == 1) {
        /* Buy in */
        BuyRecord* br = decl_book.queryBuyFirst();

        /* ????????????????????????????????????????????????????????? */
        if (br == nullptr)
            return -1; /* fully reject */

        double t_price = br->price;

        /**
         * NOTE:
         * ????????????????????????1????????????????????????????????????????????????????????????????????????????????????????????????
         */
        BuyRecord new_br = {
            order.order_id,
            t_price,
            order.volume};
        decl_book.insertBuyDecl(new_br);
    } else if (order.direction == -1) {
        /* Sell out */
        SellRecord *sr = decl_book.querySellFirst();

        if (sr == nullptr)
            return -1; /* fully reject */

        double t_price = sr->price;

        SellRecord new_sr = {
            order.order_id,
            t_price,
            order.volume};
        decl_book.insertSellDecl(new_sr);
    } else {
        ex_debug("strange order direction: %d\n", order.direction);
    }
    return 0;
}

/* Type 3 */
int StockExchange::handleFiveLevelOtherCancel(Order& order)
{
    /**
     * ?????????????????????????????????????????????
     *
     * NOTE:
     * 1. ?????????????????????????????????????????????????????????????????????????????????????????????????????????
     */
    if (order.direction == 1) {
        /* Buy in */
        int left_volume = order.volume;
        int level_count = 0;
        double previous_price = 0;
        while (true) {
            if (left_volume == 0)
                break;
            SellRecord *sr = decl_book.querySellFirst();
            if (sr == nullptr)
                break;

            if (sr->price != previous_price) {
                if (++level_count > 5)
                    break;
                previous_price = sr->price;
            }

            if (left_volume < sr->volume) {
                Trade new_trade = {
                    stk_code:   stk_code,
                    bid_id:     order.order_id,
                    ask_id:     sr->order_id,
                    price:      sr->price,
                    volume:     left_volume,
                };
                produceTrade(new_trade);

                sr->volume -= left_volume;
                left_volume -= left_volume;
            } else {
                Trade new_trade = {
                    stk_code:   stk_code,
                    bid_id:     order.order_id,
                    ask_id:     sr->order_id,
                    price:      sr->price,
                    volume:     sr->volume,
                };
                produceTrade(new_trade);

                left_volume -= sr->volume;
                decl_book.removeSellFirst();
            }
        }
        if (left_volume) {
            // ex_debug("Type3: left_volume=%d\n", left_volume);
            return left_volume; /* partial reject */
        }
    } else if (order.direction == -1) {
        /* Sell out */
        int left_volume = order.volume;
        int level_count = 0;
        double previous_price = 0;
        while (true) {
            if (left_volume == 0)
                break;
            BuyRecord *br = decl_book.queryBuyFirst();
            if (br == nullptr)
                break;

            if (br->price != previous_price) {
                if (++level_count > 5)
                    break;
                previous_price = br->price;
            }

            if (left_volume < br->volume) {
                Trade new_trade = {
                    stk_code:   stk_code,
                    bid_id:     br->order_id,
                    ask_id:     order.order_id,
                    price:      br->price,
                    volume:     left_volume,
                };
                produceTrade(new_trade);

                br->volume -= left_volume;
                left_volume -= left_volume;
            } else {
                Trade new_trade = {
                    stk_code:   stk_code,
                    bid_id:     br->order_id,
                    ask_id:     order.order_id,
                    price:      br->price,
                    volume:     br->volume,
                };
                produceTrade(new_trade);

                left_volume -= br->volume;
                decl_book.removeBuyFirst();
            }
        }
    } else {
        ex_debug("strange order direction: %d\n", order.direction);
    }

    return 0;
}

/* Type 4 */
int StockExchange::handleImmediateOtherCancel(Order& order)
{
    /* ????????????????????????????????????????????????????????????????????????????????? */
    int left_volume = order.volume;
    if (order.direction == 1) {
        /* Buy in */
        while (true) {
            if (left_volume == 0)
                break;
            SellRecord *sr = decl_book.querySellFirst();
            if (sr == nullptr)
                break;

            if (left_volume < sr->volume) {
                Trade new_trade = {
                    stk_code:   stk_code,
                    bid_id:     order.order_id,
                    ask_id:     sr->order_id,
                    price:      sr->price,
                    volume:     left_volume,
                };
                produceTrade(new_trade);

                sr->volume -= left_volume;
                left_volume -= left_volume;
            } else {
                Trade new_trade = {
                    stk_code:   stk_code,
                    bid_id:     order.order_id,
                    ask_id:     sr->order_id,
                    price:      sr->price,
                    volume:     sr->volume,
                };
                produceTrade(new_trade);

                left_volume -= sr->volume;
                decl_book.removeSellFirst();
            }
        }
        if (left_volume) {
            // ex_debug("Type4: left_volume=%d\n", left_volume);
            return left_volume; /* partial reject */
        }
    } else if (order.direction == -1) {
        /* Sell out */
        while (true) {
            if (left_volume == 0)
                break;
            BuyRecord *br = decl_book.queryBuyFirst();
            if (br == nullptr)
                break;

            if (left_volume < br->volume) {
                Trade new_trade = {
                    stk_code:   stk_code,
                    bid_id:     br->order_id,
                    ask_id:     order.order_id,
                    price:      br->price,
                    volume:     left_volume,
                };
                produceTrade(new_trade);

                br->volume -= left_volume;
                left_volume -= left_volume;
            } else {
                Trade new_trade = {
                    stk_code:   stk_code,
                    bid_id:     br->order_id,
                    ask_id:     order.order_id,
                    price:      br->price,
                    volume:     br->volume,
                };
                produceTrade(new_trade);

                left_volume -= br->volume;
                decl_book.removeBuyFirst();
            }
        }
        if (left_volume) {
            // ex_debug("Type4: left_volume=%d\n", left_volume);
            return left_volume; /* partial reject */
        }
    } else {
        ex_debug("strange order direction: %d\n", order.direction);
    }

    return 0;
}

/* Type 5 */
int StockExchange::handleWholeOrCancel(Order& order)
{
    /* ??? 4 ??????????????????????????????????????????????????????????????? */

    int left_volume = order.volume;
    if (order.direction == 1) {
        /* Buy in */

        /* ????????????????????????????????????????????????????????????????????????????????? */
        if (left_volume > decl_book.totalSellVolume())
            return -1;

        while (left_volume) {
            /* ?????????????????? left_volume */
            SellRecord *sr = decl_book.querySellFirst();
            assert(sr);

            if (left_volume < sr->volume) {
                Trade new_trade = {
                    stk_code:   stk_code,
                    bid_id:     order.order_id,
                    ask_id:     sr->order_id,
                    price:      sr->price,
                    volume:     left_volume,
                };
                produceTrade(new_trade);

                sr->volume -= left_volume;
                left_volume -= left_volume;
            } else {
                Trade new_trade = {
                    stk_code:   stk_code,
                    bid_id:     order.order_id,
                    ask_id:     sr->order_id,
                    price:      sr->price,
                    volume:     sr->volume,
                };
                produceTrade(new_trade);

                left_volume -= sr->volume;
                decl_book.removeSellFirst();
            }
        }

    } else if (order.direction == -1) {
        /* Sell out */
        if (left_volume > decl_book.totalBuyVolume())
            return -1;

        while (left_volume) {
            BuyRecord *br = decl_book.queryBuyFirst();
            assert(br);

            if (left_volume < br->volume) {
                Trade new_trade = {
                    stk_code:   stk_code,
                    bid_id:     br->order_id,
                    ask_id:     order.order_id,
                    price:      br->price,
                    volume:     left_volume,
                };
                produceTrade(new_trade);

                br->volume -= left_volume;
                left_volume -= left_volume;
            } else {
                Trade new_trade = {
                    stk_code:   stk_code,
                    bid_id:     br->order_id,
                    ask_id:     order.order_id,
                    price:      br->price,
                    volume:     br->volume,
                };
                produceTrade(new_trade);

                left_volume -= br->volume;
                decl_book.removeBuyFirst();
            }
        }
    } else {
        ex_debug("strange order direction: %d\n", order.direction);
    }

    return 0;
}

int StockExchange::commitOrder(Order& order) {
    /**
     * 1. split by order type
     * 2. handle for each type, and return status
     */
    int ret = 0;

    switch (order.type) {
        case -1: { /* Cancelled Order */
            ret = 0;
            break;
        }
        case 0: { /* ???????????? */
            ret = handleLimitOrder(order);
            break;
        }
        case 1: { /* ??????????????????????????? */
            ret = handleCounterpartyBest(order);
            break;
        }
        case 2: { /* ???????????????????????? */
            ret = handleOurBest(order);
            break;
        }
        case 3: { /* ?????????????????????????????????????????? */
            ret = handleFiveLevelOtherCancel(order);
            break;
        }
        case 4: { /* ?????????????????????????????? */
            ret = handleImmediateOtherCancel(order);
            break;
        }
        case 5: { /* ??????????????????????????? */
            ret = handleWholeOrCancel(order);
            break;
        }
        default: {
            ex_debug("exchange type: %d\n", order.type);
            return -1;
        }
    }

    return 0;
}

}  // namespace ubiquant