#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

#include "H5Cpp.h"
#include "common/config.h"
#include "common/type.hpp"
#include "utils/assertion.hpp"
#include "utils/timer.hpp"

namespace ubiquant {

enum matrix_idx {
    order_id_idx,
    direction_idx,
    type_idx,
    price_idx,
    volume_idx,
    num_matrix
};

// const H5std_string h5_prefix = "/data/100x1000x1000/";
H5std_string hook_fname;

std::vector<std::vector<H5std_string>> INPUT_FILE_NAME;
std::vector<std::vector<H5std_string>> CACHE_FILE_NAME;

bool loader_inited = false;

void init_loader() {
    hook_fname = Config::data_folder + "hook.h5";

    INPUT_FILE_NAME = std::vector<std::vector<H5std_string>>({{Config::data_folder + "order_id1.h5",
                                                               Config::data_folder + "direction1.h5",
                                                               Config::data_folder + "type1.h5",
                                                               Config::data_folder + "price1.h5",
                                                               Config::data_folder + "volume1.h5"},
                                                              {Config::data_folder + "order_id2.h5",
                                                               Config::data_folder + "direction2.h5",
                                                               Config::data_folder + "type2.h5",
                                                               Config::data_folder + "price2.h5",
                                                               Config::data_folder + "volume2.h5"}});

    CACHE_FILE_NAME = std::vector<std::vector<H5std_string>>({{Config::trade_output_folder + "order_id1.mat",
                                                               Config::trade_output_folder + "direction1.mat",
                                                               Config::trade_output_folder + "type1.mat",
                                                               Config::trade_output_folder + "price1.mat",
                                                               Config::trade_output_folder + "volume1.mat"},
                                                              {Config::trade_output_folder + "order_id2.mat",
                                                               Config::trade_output_folder + "direction2.mat",
                                                               Config::trade_output_folder + "type2.mat",
                                                               Config::trade_output_folder + "price2.mat",
                                                               Config::trade_output_folder + "volume2.mat"}});

    loader_inited = true;
}

// part should be 0 or 1
inline H5std_string get_input_fname(int part, matrix_idx idx) {
    return INPUT_FILE_NAME[part][idx];
}

inline H5std_string get_cache_fname(int part, matrix_idx idx) {
    return CACHE_FILE_NAME[part][idx];
}

const std::vector<H5std_string> DATASET_NAME = {
    "order_id",
    "direction",
    "type",
    "price",
    "volume"};

// rank 1 length 10
const H5std_string PREV_CLOSE_DATASET = "prev_close";
// (10, 100, 4)
const H5std_string HOOK_DATASET = "hook";

void dataset_read(int* data_read, const H5::DataSet& dataset, const H5::DataSpace& memspace, const H5::DataSpace& dataspace) {
    dataset.read(data_read, H5::PredType::NATIVE_INT, memspace, dataspace);
}

void dataset_read(double* data_read, const H5::DataSet& dataset, const H5::DataSpace& memspace, const H5::DataSpace& dataspace) {
    dataset.read(data_read, H5::PredType::NATIVE_DOUBLE, memspace, dataspace);
}

// load_matrix_from_file<int> -> shared_ptr<int[]>
// load_matrix_from_file<double> -> shared_ptr<double[]>
template <typename T>
std::shared_ptr<T[]> load_matrix_from_file(const H5std_string fname, const H5std_string dataset_name, const int rank, const hsize_t* count, const hsize_t* offset) {
    assert(loader_inited);

    int num_data = 1;
    for (int i = 0; i < rank; i++) {
        num_data *= count[i];
    }

    std::shared_ptr<T[]> data_read(new T[num_data]);
    memset((void*)data_read.get(), 0, sizeof(T) * num_data);
    H5::H5File file(fname, H5F_ACC_RDONLY);
    H5::DataSet dataset = file.openDataSet(dataset_name);

    H5::DataSpace dataspace = dataset.getSpace();
    int dataset_rank = dataspace.getSimpleExtentNdims();
    assert(dataset_rank == rank);

    auto dims_out = std::make_unique<hsize_t[]>(rank);
    dataspace.getSimpleExtentDims(dims_out.get(), NULL);

    std::cout << fname.c_str() << ": " << dataset_name.c_str() << " rank: " << rank << " shape: (";
    for (int i = 0; i < rank; i++) {
        std::cout << dims_out[i];
        if (i == rank - 1)
            std::cout << ")\n";
        else
            std::cout << ", ";
    }

    dataspace.selectHyperslab(H5S_SELECT_SET, count, offset);  // select in file, this api can set api

    H5::DataSpace memspace(rank, count);
    memspace.selectHyperslab(H5S_SELECT_SET, count, offset);  // select in memory

    uint64_t start = timer::get_usec();
    dataset_read(data_read.get(), dataset, memspace, dataspace);
    // read from file to memory, you can set offset in memory space
    uint64_t end = timer::get_usec();
    std::cout << "Load " << fname << " " << dataset_name << " " << num_data << " finish in " << (end - start) / 1000 << " msec" << std::endl;

    file.close();
    return data_read;
}

std::vector<std::vector<SortStruct>> load_order_id_from_file(int part) {
    // read a 500x1000x1000 matrix
    const int NX_SUB = Config::loader_nx_matrix;
    const int NY_SUB = Config::loader_ny_matrix;
    const int NZ_SUB = Config::loader_nz_matrix;
    const int RANK = 3;

    hsize_t offset[3] = {0, 0, 0};
    hsize_t count[3] = {NX_SUB, NY_SUB, NZ_SUB};
    auto data_read = load_matrix_from_file<order_id_t>(get_input_fname(part, order_id_idx), DATASET_NAME[order_id_idx], RANK, count, offset);

    uint64_t start = timer::get_usec();
    std::vector<std::vector<SortStruct>> order_id(Config::stock_num, std::vector<SortStruct>(NX_SUB * NY_SUB * NZ_SUB / Config::stock_num));
    for (int x = 0; x < NX_SUB; x++) {
        for (int y = 0; y < NY_SUB; y++) {
            for (int z = 0; z < NZ_SUB; z++) {
                order_id[x % Config::stock_num][(x / Config::stock_num) * (NY_SUB * NZ_SUB) + y * (NZ_SUB) + z].order_id = data_read[x * (NY_SUB * NZ_SUB) + y * (NZ_SUB) + z];
                order_id[x % Config::stock_num][(x / Config::stock_num) * (NY_SUB * NZ_SUB) + y * (NZ_SUB) + z].coor.set(x, y, z);
            }
        }
    }

    for (int t = 0; t < Config::stock_num; t++) {
        std::cout << "sorting " << t << std::endl;
        sort(order_id[t].begin(), order_id[t].end());
    }

    uint64_t end = timer::get_usec();
    std::cout << "Sort order_id" << part << " finish in " << (end - start) / 1000 / 1000 << " sec" << std::endl;

    // for (int t = 0; t < Config::stock_num; t++) {
    //     for (int i = 0; i < 5; i++) {
    //         std::cout << order_id[t][i].order_id << " ("
    //                   << order_id[t][i].coor.get_x() << ","
    //                   << order_id[t][i].coor.get_y() << ","
    //                   << order_id[t][i].coor.get_z() << ") | ";
    //     }
    //     std::cout << std::endl;
    // }

    return order_id;
}

std::pair<std::vector<std::unordered_map<order_id_t, HookTarget>>, std::shared_ptr<std::vector<std::unordered_map<trade_idx_t, volume_t>>>> load_hook() {
    const int NX_SUB = 10;
    const int NY_SUB = 100;
    const int NZ_SUB = 4;
    const int RANK_OUT = 3;

    hsize_t offset[3] = {0, 0, 0};
    hsize_t count[3] = {NX_SUB, NY_SUB, NZ_SUB};
    auto data_read = load_matrix_from_file<int>(hook_fname, HOOK_DATASET, RANK_OUT, count, offset);

    // using stock id and trade id to locate a trade
    std::vector<std::unordered_map<order_id_t, HookTarget>> hook(Config::stock_num, std::unordered_map<order_id_t, HookTarget>());
    auto hooked_trade = std::make_shared<std::vector<std::unordered_map<trade_idx_t, volume_t>>>(Config::stock_num, std::unordered_map<trade_idx_t, volume_t>());

    for (int x = 0; x < NX_SUB; x++) {
        for (int y = 0; y < NY_SUB; y++) {
            int stock_id = x % 10;
            int self_order_id = data_read[x * (NY_SUB * NZ_SUB) + y * (NZ_SUB)];
            int target_stk_code = data_read[x * (NY_SUB * NZ_SUB) + y * (NZ_SUB) + 1];   // start at 1
            int target_trade_idx = data_read[x * (NY_SUB * NZ_SUB) + y * (NZ_SUB) + 2];  // start at 1
            int arg = data_read[x * (NY_SUB * NZ_SUB) + y * (NZ_SUB) + 3];

            hook[stock_id][self_order_id] = {target_stk_code, target_trade_idx, arg};
            (*hooked_trade)[target_stk_code - 1][target_trade_idx] = -1;
        }
    }

    // for (int t = 0; t < Config::stock_num; t++) {
    //     for (auto it = hook[t].begin(); it != hook[t].end(); it++) {
    //         std::cout << it->first << " ("
    //                   << it->second.target_stk_code << ","
    //                   << it->second.target_trade_idx << ","
    //                   << it->second.arg << ") | ";

    //         if (distance(hook[t].begin(), it) == 4)
    //             break;
    //     }
    //     std::cout << std::endl;
    // }

    return make_pair(hook, hooked_trade);
}

std::vector<std::vector<price_t>> load_prev_close(int part) {
    hsize_t offset = 0;
    hsize_t count = Config::stock_num;

    auto data_read = load_matrix_from_file<price_t>(get_input_fname(part, price_idx), PREV_CLOSE_DATASET, 1, &count, &offset);

    std::vector<std::vector<price_t>> price_limits(2, std::vector<price_t>(10));
    for (int t = 0; t < Config::stock_num; t++) {
        price_limits[0][t] = data_read[t] - data_read[t] * 0.1;  // ALERT: *0.9 != 1 - 0.1
        price_limits[1][t] = data_read[t] + data_read[t] * 0.1;
    }

    std::cout << "prev_price\tmin\tmax" << std::endl;
    for (int t = 0; t < Config::stock_num; t++) {
        std::cout << data_read[t] << "\t" << price_limits[0][t] << "\t" << price_limits[1][t] << std::endl;
    }

    return price_limits;
}

template <typename T>
T load_single_data_from_file(int part, matrix_idx idx, Coordinates coor) {
    T data_read;

    H5::H5File file(get_input_fname(part, idx), H5F_ACC_RDONLY);
    H5::DataSet dataset = file.openDataSet(DATASET_NAME[idx]);

    H5::DataSpace dataspace = dataset.getSpace();

    hsize_t offset[3] = {coor.get_x(), coor.get_y(), coor.get_z()};
    hsize_t count[3] = {1, 1, 1};
    dataspace.selectHyperslab(H5S_SELECT_SET, count, offset);  // select in file, this api can set api

    hsize_t dimsm = 1;
    H5::DataSpace memspace(1, &dimsm);

    hsize_t offset_out = 0;
    hsize_t count_out = 1;
    memspace.selectHyperslab(H5S_SELECT_SET, &count_out, &offset_out);  // select in memory

    dataset_read(&data_read, dataset, memspace, dataspace);

    file.close();
    return data_read;
}

class OrderGenerator {
   private:
    std::vector<std::vector<std::ifstream>> ifs;
    std::vector<std::shared_ptr<order_id_t[]>> order_id_matrix;
    std::vector<std::shared_ptr<direction_t[]>> direction_matrix;
    std::vector<std::shared_ptr<type_t[]>> type_matrix;
    std::vector<std::shared_ptr<price_t[]>> price_matrix;
    std::vector<std::shared_ptr<volume_t[]>> volume_matrix;

    std::vector<int> start_idx;
    int length;

    std::vector<int> cur_partition;
    const int num_partition = 10;

   public:
    bool load_data(int stk_code_minus_one) {
        if (cur_partition[stk_code_minus_one] >= num_partition)
            return false;

        cur_partition[stk_code_minus_one]++;

        ifs[stk_code_minus_one][order_id_idx].read((char*)order_id_matrix[stk_code_minus_one].get(), sizeof(order_id_t) * length);
        ifs[stk_code_minus_one][direction_idx].read((char*)direction_matrix[stk_code_minus_one].get(), sizeof(direction_t) * length);
        ifs[stk_code_minus_one][type_idx].read((char*)type_matrix[stk_code_minus_one].get(), sizeof(type_t) * length);
        ifs[stk_code_minus_one][price_idx].read((char*)price_matrix[stk_code_minus_one].get(), sizeof(price_t) * length);
        ifs[stk_code_minus_one][volume_idx].read((char*)volume_matrix[stk_code_minus_one].get(), sizeof(volume_t) * length);

        start_idx[stk_code_minus_one] = 0;
        return true;
    }

    Order generate_order(stock_code_t stk_code) {
        stk_code--;

        if (start_idx[stk_code] >= length) {
            if(!load_data(stk_code)) {
                Order fail;
                fail.type = -1;
                return fail;
            }
        }
        int idx = start_idx[stk_code];
        Order order;
        order.stk_code = stk_code + 1;
        order.order_id = order_id_matrix[stk_code][idx];
        order.direction = direction_matrix[stk_code][idx];
        order.type = type_matrix[stk_code][idx];
        order.price = price_matrix[stk_code][idx];
        order.volume = volume_matrix[stk_code][idx];

        return order;
    }

    void commit(stock_code_t stk_code) {
        start_idx[--stk_code]++;
    }

    OrderGenerator() {
        uint64_t start = timer::get_usec();
        ifs = std::vector<std::vector<std::ifstream>>(Config::stock_num);
        for (int t = 0; t < Config::stock_num; t++) {
            for (int i = 0; i < num_matrix; i++) {
                ifs[t].emplace_back(std::ifstream(get_cache_fname(Config::partition_idx, (matrix_idx)i) + "-" + std::to_string(t), std::ios::in | std::ios::binary));
            }
        }

        length = Config::loader_nx_matrix * Config::loader_ny_matrix * Config::loader_nz_matrix / Config::stock_num / num_partition;
        start_idx.resize(Config::stock_num);
        cur_partition = std::vector<int>(Config::stock_num, 0);

        order_id_matrix.resize(Config::stock_num);
        direction_matrix.resize(Config::stock_num);
        type_matrix.resize(Config::stock_num);
        price_matrix.resize(Config::stock_num);
        volume_matrix.resize(Config::stock_num);

        for (int t = 0; t < Config::stock_num; t++) {
            order_id_matrix[t] = std::shared_ptr<order_id_t[]>(new order_id_t[length]);
            direction_matrix[t] = std::shared_ptr<direction_t[]>(new direction_t[length]);
            type_matrix[t] = std::shared_ptr<type_t[]>(new type_t[length]);
            price_matrix[t] = std::shared_ptr<price_t[]>(new price_t[length]);
            volume_matrix[t] = std::shared_ptr<volume_t[]>(new volume_t[length]);
            load_data(t);
        }

        uint64_t end = timer::get_usec();
        std::cout << "Init OrderGenerator in " << (end - start) / 1000 << " msec" << std::endl;
    }

    ~OrderGenerator() {
        for (int t = 0; t < Config::stock_num; t++) {
            for (int i = 0; i < num_matrix; i++) {
                ifs[t][i].close();
            }
        }
    }
};

}  // namespace ubiquant
