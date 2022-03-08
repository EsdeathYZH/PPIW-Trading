/* matchmaking */

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

/**
 * @brief trading rules
 *
 * 连续竞价，是指对买卖申报逐笔连续撮合的竞价方式。
 *
 * 连续竞价时，成交价的确定原则为：
 * （一）最高买入申报与最低卖出申报价格相同，以该价格为成交价；
 * （二）买入申报价格高于集中申报簿当时最低卖出申报价格时，
 *      以集中申报簿当时的最低卖出申报价格为成交价；
 * （三）卖出申报价格低于集中申报簿当时最高买入申报价格时，
 *      以集中申报簿当时的最高买入申报价格为成交价。
 */
