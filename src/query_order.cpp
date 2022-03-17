#include <iostream>

#include <common/loader.hpp>
#include <common/config.h>

#include <common/type.hpp>

namespace ubiquant {

std::vector<std::vector<Order>> fetch_order_by_config()
{
    std::vector<std::vector<Order>> order_list(11);
    OrderInfoMatrix oim;
    int part_id;

    hsize_t count[3] = {Config::loader_nx_matrix, Config::loader_ny_matrix, Config::loader_nz_matrix};
    hsize_t offset[3] = {0, 0, 0};
    const int RANK = 3;

    int file_order_count = Config::loader_nx_matrix * Config::loader_ny_matrix * Config::loader_nz_matrix / Config::stock_num;

    part_id = 0;
    auto sorted_order_id = load_order_id_from_file(part_id);
    oim.direction_matrix = load_matrix_from_file<direction_t>(get_input_fname(part_id, direction_idx), DATASET_NAME[direction_idx], RANK, count, offset);
    oim.type_matrix = load_matrix_from_file<type_t>(get_input_fname(part_id, type_idx), DATASET_NAME[type_idx], RANK, count, offset);
    oim.price_matrix = load_matrix_from_file<price_t>(get_input_fname(part_id, price_idx), DATASET_NAME[price_idx], RANK, count, offset);
    oim.volume_matrix = load_matrix_from_file<volume_t>(get_input_fname(part_id, volume_idx), DATASET_NAME[volume_idx], RANK, count, offset);

    for (int t = 0; t < 10; t++) {
        for (int i = 0; i < file_order_count; i++) {
            Order order = oim.generate_order(t + 1, sorted_order_id[t][i], Config::loader_nx_matrix, Config::loader_ny_matrix, Config::loader_nz_matrix);
            order_list[t+1].push_back(order);
            // exchange.handleSingleOrder(order);
        }
        std::cout << std::endl;
    }

    part_id = 1;
    sorted_order_id = load_order_id_from_file(part_id);
    oim.direction_matrix = load_matrix_from_file<direction_t>(get_input_fname(part_id, direction_idx), DATASET_NAME[direction_idx], RANK, count, offset);
    oim.type_matrix = load_matrix_from_file<type_t>(get_input_fname(part_id, type_idx), DATASET_NAME[type_idx], RANK, count, offset);
    oim.price_matrix = load_matrix_from_file<price_t>(get_input_fname(part_id, price_idx), DATASET_NAME[price_idx], RANK, count, offset);
    oim.volume_matrix = load_matrix_from_file<volume_t>(get_input_fname(part_id, volume_idx), DATASET_NAME[volume_idx], RANK, count, offset);

    for (int t = 0; t < 10; t++) {
        for (int i = 0; i < file_order_count; i++) {
            Order order = oim.generate_order(t + 1, sorted_order_id[t][i], Config::loader_nx_matrix, Config::loader_ny_matrix, Config::loader_nz_matrix);
            order_list[t+1].push_back(order);
            // exchange.handleSingleOrder(order);
        }
        std::cout << std::endl;
    }

    /* Consume orders */
    for (auto& ol: order_list) {
        sort(ol.begin(), ol.end(), [](auto& o1, auto& o2) {
            return o1.order_id < o2.order_id;
        });
    }

    return order_list;
}

} // namespace ubiquant


using namespace ubiquant;

int main()
{
    load_config("../config");
    init_loader();

    std::vector<std::vector<Order>> order_list = fetch_order_by_config();

    std::cout << "usage: query> <stk_code> <start> <end>" << std::endl;

    while (true) {
        std::cout << "query> ";
        int stk_code, start, end;
        std::cin >> stk_code >> start >> end;
        for (int i = start; i < end; ++i) {
            order_list[stk_code][i].print();
        }
    }

    return 0;
}
