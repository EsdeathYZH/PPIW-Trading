#include <algorithm>
#include <vector>

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
    int bid_id;
    int ask_id;
    double price;
    int volume;
};

class StockDeclarationBook {
private:
    std::vector<Order> buy_decls;
    std::vector<Order> sell_decls;
public:
    int insert_buy_decl(Order& order) {
        return -1;
    }
};

class Exchange {
private:

public:
    Exchange() {};
};

int main()
{
    return 0;
}