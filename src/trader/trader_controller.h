#pragma once

#include <mutex>
#include <unordered_map>
#include <vector>

#include "H5Cpp.h"
#include "common/config.h"
#include "common/thread.h"
#include "common/type.hpp"
#include "trader/order_sender.h"
#include "trader/trade_receiver.h"

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

class TraderController : public ubi_thread {
   public:
    TraderController();

    void load_data();

    void run() override;

    void stop_sender();
    void stop_receiver();
    void restart();
    void reset_network();

    void run_all_in_memory();

    bool check_order(Order& order, order_id_t order_id_upper_limits, stock_code_t stk_code_minus_one);

    void update_sliding_window_start(const stock_code_t stock_code, const order_id_t new_sliding_window_start);

    void update_if_hooked(const stock_code_t stock_code, const trade_idx_t trade_idx, const volume_t volume);

    inline volatile bool is_inited() { return init_finished; }

   protected:
    std::vector<std::vector<price_t>> price_limits;
    std::vector<std::unordered_map<order_id_t, HookTarget>> hook;
    std::shared_ptr<std::vector<std::unordered_map<trade_idx_t, volume_t>>> hooked_trade;
    std::vector<std::vector<SortStruct>> sorted_order_structs;

    // read a NX_SUB*NY_SUB*NZ_SUB matrix
    const int NX_SUB;
    const int NY_SUB;
    const int NZ_SUB;
    const int RANK = 3;

    hsize_t count[3];
    hsize_t offset[3] = {0, 0, 0};
    ubiquant::OrderInfoMatrix oim;

    std::vector<int> next_sorted_struct_idx;
    std::shared_ptr<SharedTradeInfo> sharedInfo;

    // order sender (exchange_num)
    std::vector<std::shared_ptr<TraderOrderSender>> order_senders_;

    // order sender (1)
    std::shared_ptr<TraderTradeReceiver> trade_receiver_;


    volatile bool init_finished = false;
};

}  // namespace ubiquant
