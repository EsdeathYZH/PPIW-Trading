#include <cstring>
#include <iostream>
#include <string>

#include "common/loader.hpp"
#include "trader/shared_trade_info.hpp"
#include "utils/timer.hpp"

int main(int argc, char *argv[]) {
    int part_id;
    std::string cache_dir;

    if (argc < 2) {
        std::cout << "Usage: ./trader part_id [cache_dir]" << std::endl;
        return 0;
    } else if (argc == 2) {
        part_id = std::stoi(std::string(argv[1]));
    } else if (argc == 3) {
        part_id = std::stoi(std::string(argv[1]));
        cache_dir = std::string(argv[2]);
    } else {
        std::cout << "Usage: ./trader part_id [cache_dir]" << std::endl;
        return 0;
    }
    std::cout << "part_id: " << part_id << std::endl;
    std::cout << "cache_dir: " << cache_dir << std::endl;

    auto price_limits = ubiquant::load_prev_close(part_id);
    auto [hook, hooked_trade] = ubiquant::load_hook();
    auto sorted_order_id = ubiquant::load_order_id_from_file(part_id);

    // read a 500x1000x1000 matrix
    const int NX_SUB = 500;
    const int NY_SUB = 1000;
    const int NZ_SUB = 1000;
    const int RANK = 3;

    hsize_t count[3] = {NX_SUB, NY_SUB, NZ_SUB};
    hsize_t offset[3] = {0, 0, 0};
    ubiquant::OrderInfoMatrix oim;
    oim.direction_matrix = ubiquant::load_matrix_from_file<ubiquant::direction_t>(ubiquant::get_fname(part_id, ubiquant::direction_idx), ubiquant::DATASET_NAME[ubiquant::direction_idx], RANK, count, offset);
    oim.type_matrix = ubiquant::load_matrix_from_file<ubiquant::type_t>(ubiquant::get_fname(part_id, ubiquant::type_idx), ubiquant::DATASET_NAME[ubiquant::type_idx], RANK, count, offset);
    oim.price_matrix = ubiquant::load_matrix_from_file<ubiquant::price_t>(ubiquant::get_fname(part_id, ubiquant::price_idx), ubiquant::DATASET_NAME[ubiquant::price_idx], RANK, count, offset);
    oim.volume_matrix = ubiquant::load_matrix_from_file<ubiquant::volume_t>(ubiquant::get_fname(part_id, ubiquant::volume_idx), ubiquant::DATASET_NAME[ubiquant::volume_idx], RANK, count, offset);

    for (int t = 0; t < ubiquant::Config::stock_num; t++) {
        for (int i = 0; i < 5; i++) {
            ubiquant::Order order = oim.generate_order(t + 1, sorted_order_id[t][i], NX_SUB, NY_SUB, NZ_SUB);
            order.print();
            std::cout << " ";
        }
        std::cout << std::endl;
    }

    std::vector<int> next_sorted_struct_idx(ubiquant::Config::stock_num, 0);
    auto sharedInfo = std::make_shared<ubiquant::SharedTradeInfo>(hooked_trade);
    // check hook and price limits
    // if order is going to abandoned, set type of order as -1

    while (ubiquant::at_work()) {
        std::vector<ubiquant::Order> order_to_send;

        for (int t = 0; t < ubiquant::Config::stock_num; t++) {
            // sending order with id less than order_id_limits
            uint64_t order_id_limits = sharedInfo->get_sliding_window_start(t + 1) + ubiquant::Config::sliding_window_size;
            int &ss_idx = next_sorted_struct_idx[t];
            while (true) {
                ubiquant::Order order = oim.generate_order(t + 1, sorted_order_id[t][ss_idx], NX_SUB, NY_SUB, NZ_SUB);

                // check order id < order_id_limits
                if (order.order_id >= order_id_limits)
                    break;

                // check if is hook
                if (hook[t].count(order.order_id)) {
                    ubiquant::HookTarget ht = hook[t][order.order_id];
                    ubiquant::volume_t v = sharedInfo->get_hooked_volume(ht.target_stk_code, ht.target_trade_idx);
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
        
        // TODO: remove this
        ubiquant::finish_work();
    }

    return 0;
}
