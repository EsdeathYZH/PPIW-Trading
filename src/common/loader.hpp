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
#include "common/type.hpp"
#include "utils/assertion.hpp"
#include "utils/timer.hpp"

using namespace std;
using namespace H5;

enum matrix_idx {
    order_id_idx,
    direction_idx,
    type_idx,
    price_idx,
    volume_idx,
    num_matrix
};

const string h5_prefix = "/data/100x1000x1000/";
const string hook_fname = h5_prefix + "hook.h5";

const vector<vector<H5std_string>> FILE_NAME = {
    {h5_prefix + "order_id1.h5",
     h5_prefix + "direction1.h5",
     h5_prefix + "type1.h5",
     h5_prefix + "price1.h5",
     h5_prefix + "volume1.h5"},
    {h5_prefix + "order_id2.h5",
     h5_prefix + "direction2.h5",
     h5_prefix + "type2.h5",
     h5_prefix + "price2.h5",
     h5_prefix + "volume2.h5"}};

// part should be 1 or 2
inline string get_fname(int part, matrix_idx idx) {
    return FILE_NAME[part - 1][idx];
}

const vector<H5std_string> DATASET_NAME = {
    "order_id",
    "direction",
    "type",
    "price",
    "volume"};

// rank 1 length 10
const H5std_string PREV_CLOSE_DATASET = "prev_close";
// (10, 100, 4)
const H5std_string HOOK_DATASET = "hook";

const string cache_dir = "/data/team-7/";

const int num_stock = 10;

void dataset_read(int* data_read, const DataSet& dataset, const DataSpace& memspace, const DataSpace& dataspace) {
    dataset.read(data_read, PredType::NATIVE_INT, memspace, dataspace);
}

void dataset_read(double* data_read, const DataSet& dataset, const DataSpace& memspace, const DataSpace& dataspace) {
    dataset.read(data_read, PredType::NATIVE_DOUBLE, memspace, dataspace);
}

// load_matrix_from_file<int> -> shared_ptr<int[]>
// load_matrix_from_file<double> -> shared_ptr<double[]>
template <typename T>
shared_ptr<T[]> load_matrix_from_file(const H5std_string fname, const H5std_string dataset_name, const int rank, const hsize_t* count, const hsize_t* offset) {
    int num_data = 1;
    for (int i = 0; i < rank; i++) {
        num_data *= count[i];
    }

    auto data_read = make_shared<T[]>(num_data);
    memset(data_read.get(), 0, sizeof(T) * num_data);
    H5File file(fname, H5F_ACC_RDONLY);
    DataSet dataset = file.openDataSet(dataset_name);

    DataSpace dataspace = dataset.getSpace();
    int dataset_rank = dataspace.getSimpleExtentNdims();
    assert(dataset_rank == rank);

    auto dims_out = make_unique<hsize_t[]>(rank);
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

    DataSpace memspace(rank, count);
    memspace.selectHyperslab(H5S_SELECT_SET, count, offset);  // select in memory

    uint64_t start = timer::get_usec();
    dataset_read(data_read.get(), dataset, memspace, dataspace);
    // read from file to memory, you can set offset in memory space
    uint64_t end = timer::get_usec();
    std::cout << "Load " << fname << " " << dataset_name << " " << num_data << " finish in " << (end - start) / 1000 << " msec" << std::endl;

    return data_read;
}

vector<vector<SortStruct>> load_order_id_from_file(int part) {
    // read a 500x1000x1000 matrix
    const int NX_SUB = 500;
    const int NY_SUB = 1000;
    const int NZ_SUB = 1000;
    const int RANK = 3;

    hsize_t offset[3] = {0, 0, 0};
    hsize_t count[3] = {NX_SUB, NY_SUB, NZ_SUB};
    auto data_read = load_matrix_from_file<order_id_t>(get_fname(part, order_id_idx), DATASET_NAME[order_id_idx], RANK, count, offset);

    uint64_t start = timer::get_usec();
    vector<vector<SortStruct>> order_id(num_stock, vector<SortStruct>(NX_SUB * NY_SUB * NZ_SUB / num_stock));
    for (int x = 0; x < NX_SUB; x++) {
        for (int y = 0; y < NY_SUB; y++) {
            for (int z = 0; z < NZ_SUB; z++) {
                order_id[x % 10][(x / 10) * (1000 * 1000) + y * 1000 + z].order_id = data_read[x * (1000 * 1000) + y * 1000 + z];
                order_id[x % 10][(x / 10) * (1000 * 1000) + y * 1000 + z].coor.set(x, y, z);
            }
        }
    }

    for (int t = 0; t < num_stock; t++) {
        sort(order_id[t].begin(), order_id[t].end());
    }

    uint64_t end = timer::get_usec();
    std::cout << "Sort order_id" << part << " finish in " << (end - start) / 1000 / 1000 << " sec" << std::endl;

    for (int t = 0; t < num_stock; t++) {
        for (int i = 0; i < 5; i++) {
            std::cout << order_id[t][i].order_id << " ("
                      << order_id[t][i].coor.get_x() << ","
                      << order_id[t][i].coor.get_y() << ","
                      << order_id[t][i].coor.get_z() << ") | ";
        }
        std::cout << std::endl;
    }

    return order_id;
}

pair<vector<unordered_map<order_id_t, HookTarget>>, shared_ptr<vector<unordered_map<trade_idx_t, volume_t>>>> load_hook() {
    const int NX_SUB = 10;
    const int NY_SUB = 100;
    const int NZ_SUB = 4;
    const int RANK_OUT = 3;

    hsize_t offset[3] = {0, 0, 0};
    hsize_t count[3] = {NX_SUB, NY_SUB, NZ_SUB};
    auto data_read = load_matrix_from_file<int>(hook_fname, HOOK_DATASET, RANK_OUT, count, offset);

    // using stock id and trade id to locate a trade
    vector<unordered_map<order_id_t, HookTarget>> hook(num_stock, unordered_map<order_id_t, HookTarget>());
    auto hooked_trade = make_shared<vector<unordered_map<trade_idx_t, volume_t>>>(num_stock, unordered_map<trade_idx_t, volume_t>());

    for (int x = 0; x < NX_SUB; x++) {
        for (int y = 0; y < NY_SUB; y++) {
            int stock_id = x % 10;
            int self_order_id = data_read[x * (NY_SUB * NZ_SUB) + y * (NZ_SUB)];
            int target_stk_code = data_read[x * (NY_SUB * NZ_SUB) + y * (NZ_SUB) + 1];   // start at 1
            int target_trade_idx = data_read[x * (NY_SUB * NZ_SUB) + y * (NZ_SUB) + 2];  // start at 1
            int arg = data_read[x * (NY_SUB * NZ_SUB) + y * (NZ_SUB) + 3];

            hook[stock_id][self_order_id] = {target_stk_code - 1, target_trade_idx, arg};
            (*hooked_trade)[target_stk_code - 1][target_trade_idx] = -1;
        }
    }

    for (int t = 0; t < num_stock; t++) {
        for (auto it = hook[t].begin(); it != hook[t].end(); it++) {
            std::cout << it->first << " ("
                      << it->second.target_stk_code << ","
                      << it->second.target_trade_idx << ","
                      << it->second.arg << ") | ";

            if (distance(hook[t].begin(), it) == 4)
                break;
        }
        std::cout << std::endl;
    }

    return make_pair(hook, hooked_trade);
}

vector<vector<price_t>> load_prev_close(int part) {
    hsize_t offset = 0;
    hsize_t count = num_stock;

    auto data_read = load_matrix_from_file<price_t>(get_fname(part, price_idx), PREV_CLOSE_DATASET, 1, &count, &offset);

    vector<vector<price_t>> price_limits(2, vector<price_t>(10));
    for (int t = 0; t < num_stock; t++) {
        price_limits[0][t] = data_read[t] - data_read[t] * 0.1;  // ALERT: *0.9 != 1 - 0.1
        price_limits[1][t] = data_read[t] + data_read[t] * 0.1;
    }

    std::cout << "prev_price\tmin\tmax" << std::endl;
    for (int t = 0; t < num_stock; t++) {
        cout << data_read[t] << "\t" << price_limits[0][t] << "\t" << price_limits[1][t] << std::endl;
    }

    return price_limits;
}

template <typename T>
T load_single_data_from_file(int part, matrix_idx idx, Coordinates coor) {
    T data_read;

    H5File file(get_fname(part, idx), H5F_ACC_RDONLY);
    DataSet dataset = file.openDataSet(DATASET_NAME[idx]);

    DataSpace dataspace = dataset.getSpace();

    hsize_t offset[3] = {coor.get_x(), coor.get_y(), coor.get_z()};
    hsize_t count[3] = {1, 1, 1};
    dataspace.selectHyperslab(H5S_SELECT_SET, count, offset);  // select in file, this api can set api

    hsize_t dimsm = 1;
    DataSpace memspace(1, &dimsm);

    hsize_t offset_out = 0;
    hsize_t count_out = 1;
    memspace.selectHyperslab(H5S_SELECT_SET, &count_out, &offset_out);  // select in memory

    dataset_read(&data_read, dataset, memspace, dataspace);

    return data_read;
}
