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

    auto price_limits = load_prev_close(part_id);
    auto [hook, hooked_trade] = load_hook();
    auto sorted_order_id = load_order_id_from_file(part_id);

    Coordinates coor;
    coor.set(1, 1, 1);
    double single_price = load_single_data_from_file<double>(part_id, price_idx, coor);

    std::cout << "load single number " << single_price << std::endl;

    // read a 500x1000x1000 matrix
    const int NX_SUB = 500;
    const int NY_SUB = 1000;
    const int NZ_SUB = 1000;
    const int RANK = 3;

    hsize_t count[3] = {NX_SUB, NY_SUB, NZ_SUB};
    hsize_t offset[3] = {0, 0, 0};
    OrderInfoMatrix oim;
    oim.direction_matrix = load_matrix_from_file<direction_t>(get_fname(part_id, direction_idx), DATASET_NAME[direction_idx], RANK, count, offset);
    oim.type_matrix = load_matrix_from_file<type_t>(get_fname(part_id, type_idx), DATASET_NAME[type_idx], RANK, count, offset);
    oim.price_matrix = load_matrix_from_file<price_t>(get_fname(part_id, price_idx), DATASET_NAME[price_idx], RANK, count, offset);
    oim.volume_matrix = load_matrix_from_file<volume_t>(get_fname(part_id, volume_idx), DATASET_NAME[volume_idx], RANK, count, offset);

    for (int t = 0; t < num_stock; t++) {
        for (int i = 0; i < 5; i++) {
            Order order = oim.generate_order(t + 1, sorted_order_id[t][i], NX_SUB, NY_SUB, NZ_SUB);
            order.print();
            std::cout << " ";
        }
        std::cout << std::endl;
    }

    return 0;
}
