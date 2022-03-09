#include <algorithm>
#include <vector>
#include <cassert>
#include <cstdio>

#define log(fmt, ...) printf("[%s:%d] " fmt, __FILE__, __LINE__, ## __VA_ARGS__)

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
};

struct Record {
    int order_id;
    double price;
    int volumn;
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

class StockDeclarationBook {
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

    SellRecord* querySellFirst() {
        if (sell_decls.empty())
            return nullptr;
        return &sell_decls[0];
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

public:
    StockExchange(int stk_code) : stk_code(stk_code), last_commit_order_id(0) {}

    int receiveOrder(Order& order) {
        /**
         * 1. push it into `not_ready_orders`
         * 2. pop and handle not_ready_orders until the head is not `last_commit_order_id + 1`
         */

         return -1;
    }

    int commitOrder(Order& order) {
        /**
         * 1. split by order type
         * 2. handle for each type, and return status
         */
        return -1;

        switch (order.type) {
            case 0: {
                /* TODO: 限价申报 */
                break;
            }
            case 1: {
                /* 对手方最优价格申报，以申报进入交易主机时集中申报簿中对手方队列的最优价格为其申报价格。*/

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
    return 0;
}