#pragma once

#include <algorithm>
#include <array>
#include <cstring>
#include <iostream>
#include <vector>

#include "H5Cpp.h"
#include "timer.hpp"
#include "type.hpp"

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
const H5std_string PREV_CLOSE = "prev_close";

const string cache_dir = "/data/team-7/";

// read a 500x1000x1000 matrix
const int NX_SUB = 500;
const int NY_SUB = 1000;
const int NZ_SUB = 1000;

const int RANK_OUT = 3;

const int num_stock = 10;

int load_order_id_from_file(int part) {
    int *data_read = new int[NX_SUB * NY_SUB * NZ_SUB];
    memset(data_read, 0, sizeof(int) * NX_SUB * NY_SUB * NZ_SUB);
    H5File file(get_fname(part, order_id_idx), H5F_ACC_RDONLY);
    DataSet dataset = file.openDataSet(DATASET_NAME[order_id_idx]);

    DataSpace dataspace = dataset.getSpace();
    int rank = dataspace.getSimpleExtentNdims();

    hsize_t dims_out[3];
    dataspace.getSimpleExtentDims(dims_out, NULL);

    printf("rank %d, shape (%llu, %llu, %llu)\n", rank, dims_out[0], dims_out[1], dims_out[2]);

    hsize_t offset[3] = {0, 0, 0};
    hsize_t count[3] = {NX_SUB, NY_SUB, NZ_SUB};
    dataspace.selectHyperslab(H5S_SELECT_SET, count, offset);  // select in file, this api can set api

    hsize_t dimsm[3] = {NX_SUB, NY_SUB, NZ_SUB};
    DataSpace memspace(RANK_OUT, dimsm);

    hsize_t offset_out[3] = {0, 0, 0};
    hsize_t count_out[3] = {NX_SUB, NY_SUB, NZ_SUB};
    memspace.selectHyperslab(H5S_SELECT_SET, count_out, offset_out);  // select in memory

    uint64_t start = timer::get_usec();
    dataset.read(data_read, PredType::NATIVE_INT, memspace, dataspace);
    // read from file to memory, you can set offset in memory space
    uint64_t end = timer::get_usec();
    std::cout << "Load order_id" << part << " finish in " << (end - start) / 1000 / 1000 << " sec" << std::endl;

    for (int i = 0; i < 5; ++i) {
        for (int j = 0; j < 5; ++j) {
            std::cout << data_read[i * (1000 * 1000) + j * 1000 + 0] << " ";
        }
        std::cout << std::endl;
    }

    std::cout << std::endl;
    for (int i = 0; i < 5; ++i) {
        for (int j = 0; j < 5; ++j) {
            std::cout << data_read[i * (1000 * 1000) + j * 1000 + 1] << " ";
        }
        std::cout << std::endl;
    }

    std::cout << std::endl;
    for (int i = 0; i < 5; ++i) {
        for (int j = 0; j < 5; ++j) {
            std::cout << data_read[i * (1000 * 1000) + j * 1000 + 2] << " ";
        }
        std::cout << std::endl;
    }

    start = timer::get_usec();
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

    end = timer::get_usec();
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

    delete[] data_read;

    return 0;
}
