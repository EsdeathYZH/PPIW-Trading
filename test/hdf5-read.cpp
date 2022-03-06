#include <iostream>
#include <cstring>
#include <sys/time.h>
#include <unistd.h>
#include <stdint.h>

#include "H5Cpp.h"
using namespace H5;

class timer {
public:
    static uint64_t get_usec() {
        struct timespec tp;
        /* POSIX.1-2008: Applications should use the clock_gettime() function
           instead of the obsolescent gettimeofday() function. */
        /* NOTE: The clock_gettime() function is only available on Linux.
           The mach_absolute_time() function is an alternative on OSX. */
        clock_gettime(CLOCK_MONOTONIC, &tp);
        return ((tp.tv_sec * 1000 * 1000) + (tp.tv_nsec / 1000));
    }
};

// #ifdef LOCAL
// const std::string orderid1_fname = "/home/xys/repo/awsome-10w/100x10x10/order_id1.h5";
// #else
// const std::string orderid1_fname = "/data/100x1000x1000/order_id1.h5";
// #endif

const H5std_string FILE_NAME("/data/100x1000x1000/order_id1.h5");
const H5std_string DATASET_NAME("order_id");

const int NX_SUB = 50;
const int NY_SUB = 1000;
const int NZ_SUB = 1000; //read a 50x5x5 matrix

const int RANK_OUT = 3;

int main(int argc, char *argv[]) {
    int *data_read = new int [50 * 1000 * 1000];
    memset(data_read, 0, sizeof(int) * 50 * 1000 * 1000);
    H5File file(FILE_NAME, H5F_ACC_RDONLY);
    DataSet dataset = file.openDataSet(DATASET_NAME);

    DataSpace dataspace = dataset.getSpace();
    int rank = dataspace.getSimpleExtentNdims();

    hsize_t dims_out[3];
    dataspace.getSimpleExtentDims(dims_out, NULL);

    printf("rank %d, shape (%llu, %llu, %llu)\n", rank, dims_out[0], dims_out[1], dims_out[2]);

    hsize_t offset[3];
    hsize_t count[3];
    hsize_t stride[3];
    offset[0] = 0;
    offset[1] = 0;
    offset[2] = 0;
    count[0] = NX_SUB;
    count[1] = NY_SUB;
    count[2] = NZ_SUB;
    stride[0] = 10;
    stride[1] = 1;
    stride[2] = 1;
    dataspace.selectHyperslab(H5S_SELECT_SET, count, offset, stride); //select in file, this api can set api

    hsize_t dimsm[3];
    dimsm[0] = NX_SUB;
    dimsm[1] = NY_SUB;
    dimsm[2] = NZ_SUB;
    DataSpace memspace(RANK_OUT, dimsm);

    hsize_t offset_out[3];
    hsize_t count_out[3];
    offset_out[0] = 0;
    offset_out[1] = 0;
    offset_out[2] = 0;
    count_out[0] = NX_SUB;
    count_out[1] = NY_SUB;
    count_out[2] = NZ_SUB;
    memspace.selectHyperslab(H5S_SELECT_SET, count_out, offset_out); // select in memory

    uint64_t start = timer::get_usec();
    dataset.read(data_read, PredType::NATIVE_INT, memspace, dataspace);
    //read from file to memory, you can set offset in memory space
    uint64_t end = timer::get_usec();

    for (int i = 0; i < 100; i++) {
        std::cout << data_read[i] << " ";
    }
    std::cout << std::endl;

    std::cout << "Finish in " << (end-start) / 1000 / 1000 << " sec" << std::endl;

    // for (int i = 0; i < 5; ++i) {
    //     for (int j = 0; j < 5; ++j) {
    //         std::cout << data_read[i * (1000 * 1000) + j * 1000 + 0] << " ";
    //     }
    //     std::cout << std::endl;
    // }

    // std::cout << std::endl;
    // for (int i = 0; i < 5; ++i) {
    //     for (int j = 0; j < 5; ++j) {
    //         std::cout << data_read[i * (1000 * 1000) + j * 1000 + 1] << " ";
    //     }
    //     std::cout << std::endl;
    // }

    // std::cout << std::endl;
    // for (int i = 0; i < 5; ++i) {
    //     for (int j = 0; j < 5; ++j) {
    //         std::cout << data_read[i * (1000 * 1000) + j * 1000 + 2] << " ";
    //     }
    //     std::cout << std::endl;
    // }
    delete[] data_read;

    return 0;
}
