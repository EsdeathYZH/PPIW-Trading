#include "common/loader.hpp"
#include "trader_controller.h"

namespace ubiquant {

extern volatile bool work_flag;

TraderController::TraderController() 
    : next_sorted_struct_idx(Config::stock_num, 0){
    
    // load order data from disk
    load_data();

    sharedInfo = std::make_shared<SharedTradeInfo>(hooked_trade);
}

void TraderController::load_data() {
    this->price_limits = load_prev_close(Config::partition_idx);
    std::tie(this->hook, this->hooked_trade) = load_hook();
    this->sorted_order_id = load_order_id_from_file(Config::partition_idx);

    oim.direction_matrix = load_matrix_from_file<direction_t>(get_fname(Config::partition_idx, direction_idx), DATASET_NAME[direction_idx], RANK, count, offset);
    oim.type_matrix = load_matrix_from_file<type_t>(get_fname(Config::partition_idx, type_idx), DATASET_NAME[type_idx], RANK, count, offset);
    oim.price_matrix = load_matrix_from_file<price_t>(get_fname(Config::partition_idx, price_idx), DATASET_NAME[price_idx], RANK, count, offset);
    oim.volume_matrix = load_matrix_from_file<volume_t>(get_fname(Config::partition_idx, volume_idx), DATASET_NAME[volume_idx], RANK, count, offset);

    for (int t = 0; t < Config::stock_num; t++) {
        for (int i = 0; i < 5; i++) {
            Order order = oim.generate_order(t + 1, sorted_order_id[t][i], NX_SUB, NY_SUB, NZ_SUB);
            order.print();
            std::cout << " ";
        }
        std::cout << std::endl;
    }
}

void TraderController::run() {
    while (work_flag) {
        std::vector<Order> order_to_send;

        for (int t = 0; t < Config::stock_num; t++) {
            // sending order with id less than order_id_limits
            uint64_t order_id_limits = sharedInfo->get_sliding_window_start(t + 1) + Config::sliding_window_size;
            int &ss_idx = next_sorted_struct_idx[t];
            while (true) {
                Order order = oim.generate_order(t + 1, sorted_order_id[t][ss_idx], NX_SUB, NY_SUB, NZ_SUB);

                // check order id < order_id_limits
                if (order.order_id >= order_id_limits)
                    break;

                // check if is hook
                if (hook[t].count(order.order_id)) {
                    HookTarget ht = hook[t][order.order_id];
                    volume_t v = sharedInfo->get_hooked_volume(ht.target_stk_code, ht.target_trade_idx);
                    if (v == -1)
                        break;
                    else if (v > ht.arg) // constraint is not met, abandon hook order
                        order.type = -1;
                }

                // check if within price limit when type == 0 限价申报
                // abandon order if price exceed limits
                if (order.type == 0 && (order.price < price_limits[0][t] || order.price > price_limits[1][t]))
                    order.type = -1;

                ss_idx++;
                order_to_send.push_back(order);
            }
        }

        // TODO: send order

    }
}

}  // namespace ubiquant

