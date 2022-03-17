#include "trader_controller.h"

#include "common/global.hpp"
#include "common/loader.hpp"
#include "order_sender.h"

namespace ubiquant {

extern volatile bool work_flag;

TraderController::TraderController()
    : next_sorted_struct_idx(Config::stock_num, 0), NX_SUB(Config::loader_nx_matrix), NY_SUB(Config::loader_ny_matrix), NZ_SUB(Config::loader_nz_matrix) {
    
    // init order sender & trade receiver
    trade_receiver_ = std::make_shared<TraderTradeReceiver>();
    for (int i = 0; i < Config::exchange_num; i++) {
        order_senders_.push_back(std::make_shared<TraderOrderSender>(i));
    }

    // start sender & recevier
    trade_receiver_->start();
    for (int i = 0; i < Config::exchange_num; i++) {
        order_senders_[i]->start();
    }

    // load order data from disk
    count[0] = NX_SUB;
    count[1] = NY_SUB;
    count[2] = NZ_SUB;
    load_data();

    // init shared info
    sharedInfo = std::make_shared<SharedTradeInfo>(hooked_trade);

    init_finished = true;
}

void TraderController::update_sliding_window_start(const stock_code_t stock_code, const order_id_t new_sliding_window_start) {
    sharedInfo->update_sliding_window_start(stock_code, new_sliding_window_start);
}

void TraderController::update_if_hooked(const stock_code_t stock_code, const trade_idx_t trade_idx, const volume_t volume) {
    sharedInfo->update_if_hooked(stock_code, trade_idx, volume);
}

void TraderController::load_data() {
    init_loader();

    this->price_limits = load_prev_close(Config::partition_idx);
    std::tie(this->hook, this->hooked_trade) = load_hook();
    this->sorted_order_id = load_order_id_from_file(Config::partition_idx);

    oim.direction_matrix = load_matrix_from_file<direction_t>(get_fname(Config::partition_idx, direction_idx), DATASET_NAME[direction_idx], RANK, count, offset);
    oim.type_matrix = load_matrix_from_file<type_t>(get_fname(Config::partition_idx, type_idx), DATASET_NAME[type_idx], RANK, count, offset);
    oim.price_matrix = load_matrix_from_file<price_t>(get_fname(Config::partition_idx, price_idx), DATASET_NAME[price_idx], RANK, count, offset);
    oim.volume_matrix = load_matrix_from_file<volume_t>(get_fname(Config::partition_idx, volume_idx), DATASET_NAME[volume_idx], RANK, count, offset);

    // for (int t = 0; t < Config::stock_num; t++) {
    //     for (int i = 0; i < 5; i++) {
    //         Order order = oim.generate_order(t + 1, sorted_order_id[t][i], NX_SUB, NY_SUB, NZ_SUB);
    //         order.print();
    //         std::cout << " ";
    //     }
    //     std::cout << std::endl;
    // }
}

void TraderController::run() {
    std::vector<Order> order_to_send;
    order_to_send.reserve(Config::stock_num * (Config::sliding_window_size + 17));
    while (work_flag) {
        order_to_send.clear();

        for (int t = 0; t < Config::stock_num; t++) {
            // sending order with id less than order_id_limits
            uint64_t order_id_limits = sharedInfo->get_sliding_window_start(t + 1) + Config::sliding_window_size;

            for (int& ss_idx = next_sorted_struct_idx[t]; ss_idx < sorted_order_id[t].size(); ss_idx++) {
                Order order = oim.generate_order(t + 1, sorted_order_id[t][ss_idx], NX_SUB, NY_SUB, NZ_SUB);

                // check order id < order_id_limits
                if (order.order_id >= order_id_limits)
                    break;

                // check if is hook
                if (hook[t].count(order.order_id)) {
                    HookTarget ht = hook[t][order.order_id];
                    volume_t v = sharedInfo->get_hooked_volume(ht.target_stk_code, ht.target_trade_idx);
                    if (v == -1)
                        break;            // hook is not ready yet
                    else if (v > ht.arg)  // constraint is not met, abandon hook order
                        order.type = -1;
                }

                // check if within price limit when type == 0 限价申报
                // abandon order if price exceed limits
                if (order.type == 0 && (order.price < price_limits[0][t] || order.price > price_limits[1][t]))
                    order.type = -1;

                order_to_send.push_back(order);
            }
        }

        // send order
        for (auto& order : order_to_send) {
            int idx = order.stk_code % Config::exchange_num;
            order_senders_[idx]->put_order(order);
        }
    }
}

}  // namespace ubiquant
