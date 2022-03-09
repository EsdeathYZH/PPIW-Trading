#include <algorithm>
#include <vector>
#include <cassert>
#include <cstdio>
#include <iostream>

#define log(fmt, ...) printf("[%s:%d] " fmt, __FILE__, __LINE__, ## __VA_ARGS__)

#define EXCHANGE_DEBUG
#ifdef EXCHANGE_DEBUG
    #define ex_debug log
#else
    #define ex_debug(fmt, ...) do {} while (0)
#endif

/* TODO: move to .h */
struct Order {
    int stk_code;
    int order_id;
    int direction;
    int type;
    double price; /* only used for type 0 */
    int volume;
};

struct Trade {
    int stk_code;
    int bid_id; /* buyer's order_id */
    int ask_id; /* seller's order_id */
    double price;
    int volume;
} __attribute__((packed));

struct Record {
    int order_id;
    double price;
    int volume;
};

struct BuyRecord : public Record {
    bool operator<(BuyRecord& br2) {
        if (price < br2.price)
            return true;
        return order_id < br2.order_id;
    };
};

struct SellRecord : public Record {
    bool operator<(SellRecord& sr2) {
        if (price > sr2.price)
            return true;
        return order_id < sr2.order_id;
    };
};

/* for debugging */
void printRecord(Record& r) {
    printf("order_id:%d\tprice:%.2f\tvolume:%d\n", r.order_id, r.price, r.volume);
}

void printRecordList(std::vector<Record>& rv) {
    for (auto& r: rv) {
        printRecord(r);
    }
}

class StockDeclarationBook {
/**
 * @brief 集中申报簿
 *
 * `buy_decls` 按照 price 降序（买价越高越优先）
 * `sell_decls` 按照 price 升序（卖价越低越优先）
 * 两者均可能为空，当两者均不为空时，买一价 严格小于 卖一价
 *
 * NOTE: 应当注意 price 的排序是不严格的，相邻两个 Record 的 price 可能相同
 *
 */
private:
    std::vector<BuyRecord> buy_decls;
    std::vector<SellRecord> sell_decls;
public:
    StockDeclarationBook () {};

    int insertBuyDecl(BuyRecord& br) {
        buy_decls.push_back(br);
        sort(buy_decls.begin(), buy_decls.end());
        return 0;
    }

    int insertSellDecl(SellRecord& sr) {
        sell_decls.push_back(sr);
        sort(sell_decls.begin(), sell_decls.end());
        return 0;
    }

    BuyRecord* queryBuyFirst() {
        if (buy_decls.empty())
            return nullptr;
        return &buy_decls[0];
    }

    void removeBuyFirst() {
        assert(!buy_decls.empty());
        buy_decls.erase(buy_decls.begin());
    }

    SellRecord* querySellFirst() {
        if (sell_decls.empty())
            return nullptr;
        return &sell_decls[0];
    }

    void removeSellFirst() {
        assert(!sell_decls.empty());
        sell_decls.erase(sell_decls.begin());
    }

    int totalBuyVolume() {
        int ret = 0;
        for (auto& br: buy_decls)
            ret += br.volume;
        return ret;
    }

    int totalSellVolume() {
        int ret = 0;
        for (auto& sr: sell_decls)
            ret += sr.volume;
        return ret;
    }
};

class StockExchange {
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
    StockExchange(int stk_code) : stk_code(stk_code), last_commit_order_id(0) {}

    int receiveOrder(Order& order) {
        /* sanity check */
        assert(order.stk_code == stk_code);

        int ret = 0;

        /**
         * 1. push it into `not_ready_orders`
         * 2. pop and handle not_ready_orders until the head is not `last_commit_order_id + 1`
         */

        /* `not_ready_orders` should maintain as heap */
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
                log("error value: %d\n", ret);
                break;
            }

            /* Erase commited order */
            std::pop_heap(not_ready_orders.begin(), not_ready_orders.end(), orderGtById);
            not_ready_orders.pop_back();

            /* Increase last_commit_id */
            last_commit_order_id++;
        }

        return ret;
    }

    int commitOrder(Order& order) {
        /**
         * 1. split by order type
         * 2. handle for each type, and return status
         */

        switch (order.type) {
            case 0: {
                /* TODO: 限价申报 */
                break;
            }
            case 1: { /* 对手方最优价格申报 */

                /* 以申报进入交易主机时集中申报簿中对手方队列的最优价格为其申报价格。*/
                if (order.direction == 1) {
                    /* Buy in */
                    SellRecord* sr = decl_book.querySellFirst();

                    /* 集中申报簿中，对手方（卖方）无申报：直接撤回此申报 */
                    if (sr == nullptr)
                        return -1;

                    double t_price = sr->price;
                    int left_volume = order.volume;

                    /* NOTE: 可能需要比对多组价格相同的 SellRecord */
                    while (left_volume) {
                        if (left_volume < sr->volume) {
                            /* 当前申报可以完全处理，卖一还有剩余 */
                            Trade new_trade;
                            new_trade.stk_code = stk_code;
                            new_trade.bid_id = order.order_id; /* buyer's order_id */
                            new_trade.ask_id = sr->order_id; /* seller's order_id */
                            new_trade.price = t_price;
                            new_trade.volume = left_volume;
                            trade_list.push_back(new_trade);

                            left_volume -= left_volume;
                            sr->volume -= left_volume;
                        } else {
                            /* 卖一将被完全处理，当前申报继续 */
                            Trade new_trade;
                            new_trade.stk_code = stk_code;
                            new_trade.bid_id = order.order_id;
                            new_trade.ask_id = sr->order_id;
                            new_trade.price = t_price;
                            new_trade.volume = sr->volume;
                            trade_list.push_back(new_trade);

                            left_volume -= sr->volume;
                            decl_book.removeSellFirst();

                            sr = decl_book.querySellFirst();
                            if (sr == nullptr || sr->price != t_price) {
                                /* 卖一价格不足交易，将剩余部分以限价形式记录在集中申报簿中 */
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
                            Trade new_trade;
                            new_trade.stk_code = stk_code;
                            new_trade.bid_id = br->order_id;
                            new_trade.ask_id = order.order_id;
                            new_trade.price = t_price;
                            new_trade.volume = left_volume;
                            trade_list.push_back(new_trade);

                            left_volume -= left_volume;
                            br->volume -= left_volume;
                        } else {
                            Trade new_trade;
                            new_trade.stk_code = stk_code;
                            new_trade.bid_id = br->order_id;
                            new_trade.ask_id = order.order_id;
                            new_trade.price = t_price;
                            new_trade.volume = br->volume;
                            trade_list.push_back(new_trade);

                            left_volume -= br->volume;
                            decl_book.removeBuyFirst();

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
                    log("strange order direction: %d\n", order.direction);
                }
                break;
            }
            case 2: { /* 本方最优价格申报 */
                /* 以申报进入交易主机时集中申报簿中本方队列的最优价格为其申报价格。*/
                if (order.direction == 1) {
                    /* Buy in */
                    BuyRecord* br = decl_book.queryBuyFirst();

                    /* 如果本方（买方）队列为空，撤销当前申报 */
                    if (br == nullptr)
                        return -1; /* fully reject */

                    double t_price = br->price;

                    /**
                     * NOTE:
                     * 由于本方第一（买1）都还不能成交，因而该申报会直接进入集中申报簿，不会产生新的交易
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
                    log("strange order direction: %d\n", order.direction);
                }
                break;
            }
            case 3: { /* 最优五档即时成交剩余撤销申报 */

                /**
                 * 依次与对手方前五档价格进行成交
                 *
                 * NOTE:
                 * 1. 价格相同但是id不同的属于两档
                 * 2. 即使第五档与第六档价位相同，也不再考虑第六档的成交情况，直接撤销
                 */
                if (order.direction == 1) {
                    /* Buy in */
                    int left_volume = order.volume;
                    for (int i = 0; i < 5; ++i) {
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
                                volume:     left_volume
                            };
                            trade_list.push_back(new_trade);

                            left_volume -= left_volume;
                            sr->volume -= left_volume;
                        } else {
                            Trade new_trade = {
                                stk_code:   stk_code,
                                bid_id:     order.order_id,
                                ask_id:     sr->order_id,
                                price:      sr->price,
                                volume:     sr->volume
                            };
                            trade_list.push_back(new_trade);

                            left_volume -= sr->volume;
                            decl_book.removeSellFirst();
                        }
                    }
                    if (left_volume) {
                        ex_debug("Type3: left_volume=%d\n", left_volume);
                        return left_volume; /* partial reject */
                    }
                } else if (order.direction == -1) {
                    /* Sell out */
                    int left_volume = order.volume;
                    for (int i = 0; i < 5; ++i) {
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
                                volume:     left_volume
                            };
                            trade_list.push_back(new_trade);

                            left_volume -= left_volume;
                            br->volume -= left_volume;
                        } else {
                            Trade new_trade = {
                                stk_code:   stk_code,
                                bid_id:     br->order_id,
                                ask_id:     order.order_id,
                                price:      br->price,
                                volume:     br->volume
                            };
                            trade_list.push_back(new_trade);

                            left_volume -= br->volume;
                            decl_book.removeBuyFirst();
                        }
                    }
                } else {
                    log("strange order direction: %d\n", order.direction);
                }
                break;
            }
            case 4: { /* 即时成交剩余撤销申报 */
                /* 匹配对手方所有申报并交易，如果对手方交易空，则撤销剩余 */
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
                                volume:     left_volume
                            };
                            trade_list.push_back(new_trade);

                            left_volume -= left_volume;
                            sr->volume -= left_volume;
                        } else {
                            Trade new_trade = {
                                stk_code:   stk_code,
                                bid_id:     order.order_id,
                                ask_id:     sr->order_id,
                                price:      sr->price,
                                volume:     sr->volume
                            };
                            trade_list.push_back(new_trade);

                            left_volume -= sr->volume;
                            decl_book.removeSellFirst();
                        }
                    }
                    if (left_volume) {
                        ex_debug("Type4: left_volume=%d\n", left_volume);
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
                                volume:     left_volume
                            };
                            trade_list.push_back(new_trade);

                            left_volume -= left_volume;
                            br->volume -= left_volume;
                        } else {
                            Trade new_trade = {
                                stk_code:   stk_code,
                                bid_id:     br->order_id,
                                ask_id:     order.order_id,
                                price:      br->price,
                                volume:     br->volume
                            };
                            trade_list.push_back(new_trade);

                            left_volume -= br->volume;
                            decl_book.removeBuyFirst();
                        }
                    }
                    if (left_volume) {
                        ex_debug("Type4: left_volume=%d\n", left_volume);
                        return left_volume; /* partial reject */
                    }
                } else {
                    log("strange order direction: %d\n", order.direction);
                }
                break;
            }
            case 5: { /* 全额成交或撤销申报 */
                /* 与 4 类似，但是如果一旦不能全额成交，则整个撤回 */

                int left_volume = order.volume;
                if (order.direction == 1) {
                    /* Buy in */

                    /* 如果当前申报的总量大于了集中申报簿中对手方的总量，撤回 */
                    if (left_volume > decl_book.totalSellVolume())
                        return -1;

                    while (left_volume) {
                        /* 保证可以清零 left_volume */
                        SellRecord *sr = decl_book.querySellFirst();
                        assert(sr);

                        if (left_volume < sr->volume) {
                            Trade new_trade = {
                                stk_code:   stk_code,
                                bid_id:     order.order_id,
                                ask_id:     sr->order_id,
                                price:      sr->price,
                                volume:     left_volume
                            };
                            trade_list.push_back(new_trade);

                            left_volume -= left_volume;
                            sr->volume -= left_volume;
                        } else {
                            Trade new_trade = {
                                stk_code:   stk_code,
                                bid_id:     order.order_id,
                                ask_id:     sr->order_id,
                                price:      sr->price,
                                volume:     sr->volume
                            };
                            trade_list.push_back(new_trade);

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
                                volume:     left_volume
                            };
                            trade_list.push_back(new_trade);

                            left_volume -= left_volume;
                            br->volume -= left_volume;
                        } else {
                            Trade new_trade = {
                                stk_code:   stk_code,
                                bid_id:     br->order_id,
                                ask_id:     order.order_id,
                                price:      br->price,
                                volume:     br->volume
                            };
                            trade_list.push_back(new_trade);

                            left_volume -= br->volume;
                            decl_book.removeBuyFirst();
                        }
                    }
                } else {
                    log("strange order direction: %d\n", order.direction);
                }
                break;
            }
            default: {
                log("exchange type: %d\n", order.type);
                return -1;
            }
        }

        return 0;
    }
};

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

int main()
{
    std::cout << "sizeof(Trade): " << sizeof(Trade) << std::endl;
    return 0;
}