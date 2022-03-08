#include <algorithm>
#include <vector>
#include <cassert>

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

class StockDeclarationBook {
private:
    int stk_code;
    int last_commit_order_id;
    std::vector<Order> buy_decls;
    std::vector<Order> sell_decls;
    static bool __buyOrderLt(Order& o1, Order& o2) {
        return o1.price < o2.price;
    };
    static bool __sellOrderLt(Order& o1, Order& o2) {
        return o1.price > o2.price;
    };
public:
    StockDeclarationBook (int stk_code) : stk_code(stk_code) {};

    int insertBuyDecl(Order& order) {
        buy_decls.push_back(order);
        sort(buy_decls.begin(), buy_decls.end(), __buyOrderLt);
        return 0;
    }

    int insertSellDecl(Order& order) {
        sell_decls.push_back(order);
        sort(sell_decls.begin(), sell_decls.end(), __sellOrderLt);
        return 0;
    }
};

class Exchange {
private:
    StockDeclarationBook declBook[11]; /* [0] is not used */
public:
    Exchange() {};

    int handleSingleOrder(Order& order) {
        /* sanity check */
        assert(order.order_id >= 1 && order.order_id <= 10);

        switch (order.type) {
            case 0: {
                /* TODO: 限价申报 */
                break;
            }
            case 1: {

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

int main()
{
    return 0;
}