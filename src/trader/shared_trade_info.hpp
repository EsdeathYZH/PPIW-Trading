#pragma once

#include <mutex>
#include <unordered_map>
#include <vector>

#include "common/config.hpp"
#include "common/type.hpp"

namespace ubiquant {

// Usage: auto sti = std::make_shared<SharedTradeInfo>(hooked_trade);
class SharedTradeInfo {
    mutable std::mutex m;
    std::vector<order_id_t> sliding_window_start;
    std::shared_ptr<std::vector<std::unordered_map<trade_idx_t, volume_t>>> hooked_trade;

   public:
    void update_sliding_window_start(const stock_code_t stock_code, const order_id_t new_sliding_window_start) {
        std::lock_guard<std::mutex> guard(m);
        assert(new_sliding_window_start >= sliding_window_start[stock_code - 1]);
        sliding_window_start[stock_code - 1] = new_sliding_window_start;
    }

    order_id_t get_sliding_window_start(const stock_code_t stock_code) const {
        std::lock_guard<std::mutex> guard(m);
        return sliding_window_start[stock_code - 1];
    }

    void update_if_hooked(const stock_code_t stock_code, const trade_idx_t trade_idx, const volume_t volume) {
        std::lock_guard<std::mutex> guard(m);
        if ((*hooked_trade)[stock_code - 1].count(trade_idx)) {
            assert((*hooked_trade)[stock_code - 1][trade_idx] == -1);  // only update once
            (*hooked_trade)[stock_code - 1][trade_idx] = volume;
        }
    }

    // for hooked trade, return -1 if not ready, else return volume of trade
    int get_hooked_volume(const stock_code_t stock_code, const trade_idx_t trade_idx) const {
        std::lock_guard<std::mutex> guard(m);
        if ((*hooked_trade)[stock_code - 1].count(trade_idx)) {
            return (*hooked_trade)[stock_code - 1][trade_idx];
        }
        std::cout << "Try get unhooked trade, stock code: " << stock_code << " trade idx: " << trade_idx << std::endl;
        assert(false);
        return -1;
    }

    SharedTradeInfo(std::shared_ptr<std::vector<std::unordered_map<trade_idx_t, volume_t>>> in_hooked_trade) : hooked_trade(in_hooked_trade) {
        sliding_window_start = std::vector<order_id_t>(Config::stock_num, 1);
    }
};

}  // namespace ubiquant
