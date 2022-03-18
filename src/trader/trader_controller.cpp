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

    if (Config::load_mode == 0) {
        auto sorted_order_id = load_order_id_from_file(Config::partition_idx);

        auto direction_matrix = load_matrix_from_file<direction_t>(get_input_fname(Config::partition_idx, direction_idx), DATASET_NAME[direction_idx], RANK, count, offset);
        auto type_matrix = load_matrix_from_file<type_t>(get_input_fname(Config::partition_idx, type_idx), DATASET_NAME[type_idx], RANK, count, offset);
        auto price_matrix = load_matrix_from_file<price_t>(get_input_fname(Config::partition_idx, price_idx), DATASET_NAME[price_idx], RANK, count, offset);
        auto volume_matrix = load_matrix_from_file<volume_t>(get_input_fname(Config::partition_idx, volume_idx), DATASET_NAME[volume_idx], RANK, count, offset);

        // output sorted matrix cache
        uint64_t start = timer::get_usec();
        std::vector<std::vector<std::ofstream>> ofs(Config::stock_num);
        for (int t = 0; t < Config::stock_num; t++) {
            for (int i = 0; i < num_matrix; i++) {
                ofs[t].emplace_back(std::ofstream(get_cache_fname(Config::partition_idx, (matrix_idx)i) + "-" + std::to_string(t), std::ios::out | std::ios::binary));
            }
        }

        const int NX = Config::loader_nx_matrix;
        const int NY = Config::loader_ny_matrix;
        const int NZ = Config::loader_nz_matrix;
        const uint64_t num_order = NX * NY * NZ / Config::stock_num;
        for (int t = 0; t < Config::stock_num; t++) {
            std::cout << "output cache " << t << std::endl;
            for (int i = 0; i < num_order; i++) {
                ofs[t][order_id_idx].write((char*)&sorted_order_id[t][i].order_id, sizeof(order_id_t));

                int x = sorted_order_id[t][i].coor.get_x(), y = sorted_order_id[t][i].coor.get_y(), z = sorted_order_id[t][i].coor.get_z();
                assert((0 <= x && x < NX) && (0 <= y && y < NY) && (0 <= z && z < NZ));
                assert(t == x % Config::stock_num);

                ofs[t][direction_idx].write((char*)&direction_matrix[x * (NY * NZ) + y * (NZ) + z], sizeof(direction_t));
                ofs[t][type_idx].write((char*)&type_matrix[x * (NY * NZ) + y * (NZ) + z], sizeof(type_t));
                ofs[t][price_idx].write((char*)&price_matrix[x * (NY * NZ) + y * (NZ) + z], sizeof(price_t));
                ofs[t][volume_idx].write((char*)&volume_matrix[x * (NY * NZ) + y * (NZ) + z], sizeof(volume_t));
            }
        }
        for (int t = 0; t < Config::stock_num; t++) {
            for (int i = 0; i < num_matrix; i++) {
                ofs[t][i].close();
            }
        }
        uint64_t end = timer::get_usec();
        std::cout << "Finish dump matrix to cache file in " << (end - start) / 1000 << " msec" << std::endl;
    } else if (Config::load_mode == 2) {
        this->sorted_order_structs = load_order_id_from_file(Config::partition_idx);

        oim.direction_matrix = load_matrix_from_file<direction_t>(get_input_fname(Config::partition_idx, direction_idx), DATASET_NAME[direction_idx], RANK, count, offset);
        oim.type_matrix = load_matrix_from_file<type_t>(get_input_fname(Config::partition_idx, type_idx), DATASET_NAME[type_idx], RANK, count, offset);
        oim.price_matrix = load_matrix_from_file<price_t>(get_input_fname(Config::partition_idx, price_idx), DATASET_NAME[price_idx], RANK, count, offset);
        oim.volume_matrix = load_matrix_from_file<volume_t>(get_input_fname(Config::partition_idx, volume_idx), DATASET_NAME[volume_idx], RANK, count, offset);
    }

    // for (int t = 0; t < Config::stock_num; t++) {
    //     for (int i = 0; i < 5; i++) {
    //         Order order = oim.generate_order(t + 1, sorted_order_id[t][i], NX_SUB, NY_SUB, NZ_SUB);
    //         order.print();
    //         std::cout << " ";
    //     }
    //     std::cout << std::endl;
    // }
}

bool TraderController::check_order(Order& order, order_id_t order_id_upper_limits, stock_code_t stk_code_minus_one) {
    // check order id < order_id_limits
    if (order.order_id >= order_id_upper_limits)
        return false;

    // check if is hook
    if (hook[stk_code_minus_one].count(order.order_id)) {
        HookTarget ht = hook[stk_code_minus_one][order.order_id];
        volume_t v = sharedInfo->get_hooked_volume(ht.target_stk_code, ht.target_trade_idx);
        if (v == -1)
            return false;     // hook is not ready yet
        else if (v > ht.arg)  // constraint is not met, abandon hook order
            order.type = -1;
    }

    // check if within price limit when type == 0 限价申报
    // abandon order if price exceed limits
    if (order.type == 0 && (order.price < price_limits[0][stk_code_minus_one] || order.price > price_limits[1][stk_code_minus_one]))
        order.type = -1;

    return true;
}

void TraderController::run_all_in_memory() {
    std::vector<Order> order_to_send;
    order_to_send.reserve(Config::stock_num * (Config::sliding_window_size + 7));
    while (work_flag) {
        order_to_send.clear();

        for (int t = 0; t < Config::stock_num; t++) {
            // sending order with id less than order_id_limits
            uint64_t order_id_limits = sharedInfo->get_sliding_window_start(t + 1) + Config::sliding_window_size;

            for (int& ss_idx = next_sorted_struct_idx[t]; ss_idx < sorted_order_structs[t].size(); ss_idx++) {
                Order order = oim.generate_order(t + 1, sorted_order_structs[t][ss_idx], NX_SUB, NY_SUB, NZ_SUB);

                if (!check_order(order, order_id_limits, t))
                    break;

                order_to_send.push_back(order);
            }
        }

        // send order
        for (auto& order : order_to_send) {
            int idx = order.stk_code % Config::exchange_num;
            order_senders_[idx]->put_order(order);
        }

        // release memory
        // std::vector<Order>().swap(order_to_send);
    }
}

void TraderController::run() {
    if (Config::load_mode == 2) {
        run_all_in_memory();
        return;
    }

    std::vector<Order> order_to_send;
    order_to_send.reserve(Config::stock_num * (Config::sliding_window_size + 7));
    OrderGenerator orderGen;
    while (work_flag) {
        order_to_send.clear();

        for (int t = 0; t < Config::stock_num; t++) {
            // sending order with id less than order_id_limits
            uint64_t order_id_limits = sharedInfo->get_sliding_window_start(t + 1) + Config::sliding_window_size;

            while (true) {
                Order order = orderGen.generate_order(t + 1);
                // order.print();
                if (order.type == -1)  // no more order for this stock code
                    break;

                if (!check_order(order, order_id_limits, t))
                    break;

                orderGen.commit(t + 1);
                order_to_send.push_back(order);
            }
        }

        // send order
        for (auto& order : order_to_send) {
            int idx = order.stk_code % Config::exchange_num;
            order_senders_[idx]->put_order(order);
        }

        // release memory
        // std::vector<Order>().swap(order_to_send);
    }
}

}  // namespace ubiquant
