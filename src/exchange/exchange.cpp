#include "exchange.h"

namespace ubiquant {

Exchange::Exchange() {
    // init stock code (HALF)
    std::vector<int> stk_codes;
    // NOTICE: stk_code starts from 1
    for(int i = 1; i <= Config::stock_num; i++) {
        if(i % Config::exchange_num == Config::partition_idx) {
            stk_codes.push_back(i);
        }
    }

    for (auto code : stk_codes) {
        stock_exchange[code] = std::make_shared<StockExchange>(code);
        order_buffer.emplace(std::make_pair(code, Config::sliding_window_size));
    }
};

Exchange::~Exchange() {
    for (auto& [code, exchange] : stock_exchange) {
        exchange->join();
    }
}

void Exchange::start() {
    for (auto& [code, exchange] : stock_exchange) {
        exchange->start();
    }
}

// Order receiver will call this function
void Exchange::receiveOrder(Order& order) {
    // NOTICE: order id starts from 1
    int window_idx = (order.order_id-1) % Config::sliding_window_size;
    try {
        order_buffer.at(order.stk_code).put(order, window_idx);
    }
    catch(...) {
        std::cerr << "throwing an exception when at: stk code=" << order.stk_code << std::endl;
        order.print();
    }
}

// Stock exchange will call this function
std::vector<Order> Exchange::comsumeOrder(int stk_code) {
    std::vector<Order> orders;
    Order order;
    while(order_buffer[stk_code].poll(order)) {
        orders.push_back(order);
    }
    // generate order ack and push into msg queue
    if(!orders.empty()) {
        OrderAck ack;
        ack.order_id = orders.back().order_id;
        ack.stk_code = stk_code;
        Global<ExchangeTradeSender>::Get()->put_order_ack(ack);
    }
    return orders;
}

// Stock exchange will call this function
void Exchange::produceTrade(Trade& new_trade) {
    Global<ExchangeTradeSender>::Get()->put_trade(new_trade);
}

/* For local processing */
int Exchange::handleSingleOrder(Order& order) {
    /* sanity check */
    assert(order.stk_code >= 1 && order.stk_code <= 10);

    order.print();
    int ret = stock_exchange[order.stk_code]->receiveOrder(order);
    // if (ret != 0) {
    //     log("error number: %d\n", ret);
    // }

    return 0;

}

}  // namespace ubiquant