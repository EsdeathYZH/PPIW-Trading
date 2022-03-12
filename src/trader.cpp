#include <cstring>
#include <iostream>
#include <string>

#include "common/loader.hpp"
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

    return 0;
}
