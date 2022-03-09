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
        return price < br2.price;
    };
};

struct SellRecord : public Record {
    bool operator<(SellRecord& sr2) {
        return price > sr2.price;
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
                                BuyRecord new_br;
                                new_br.price = t_price;
                                new_br.volume = left_volume;
                                decl_book.insertBuyDecl(new_br);

                                left_volume -= left_volume;
                            }
                        }
                    }


                }
                break;
            }
            case 2: {

                break;
            }
            case 3: {

                break;
            }
            case 4: {

                break;
            }
            case 5: {

                break;
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