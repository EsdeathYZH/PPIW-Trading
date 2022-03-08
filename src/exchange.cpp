#include <algorithm>

/* TODO: move to .h */
struct order {
    int stk_code;
    int order_id;
    int direction;
    int type;
    double price;
    int volume;
};

struct trade {
    int stk_code;
    int bid_id;
    int ask_id;
    double price;
    int volume;
};

