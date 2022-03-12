#pragma once

#define log(fmt, ...) printf("[%s:%d] " fmt, __FILE__, __LINE__, ## __VA_ARGS__)

#define EXCHANGE_DEBUG
#ifdef EXCHANGE_DEBUG
    #define ex_debug log
#else
    #define ex_debug(fmt, ...) do {} while (0)
#endif

#include "common/type.hpp"
#include <cstdio>
#include <vector>

namespace ubiquant {

static void printTrade(Trade *t) {
    printf("[%d] %d <- %d\t%.2f\t* %d\n",
        t->stk_code,
        t->bid_id,
        t->ask_id,
        t->price,
        t->volume);
}

static bool diffTradeList(std::vector<Trade>& v1, std::vector<Trade>& v2)
{
    int size1 = v1.size();
    int size2 = v2.size();
    if (size1 != size2) {
        printf("size1=%d, size2=%d\n", size1, size2);
        return false;
    }

    for (int i = 0; i < size1; ++i) {
        bool valid = true;
        valid = valid && (v1[i].stk_code == v2[i].stk_code);
        valid = valid && (v1[i].bid_id == v2[i].bid_id);
        valid = valid && (v1[i].ask_id == v2[i].ask_id);
        valid = valid && (v1[i].price == v2[i].price);
        valid = valid && (v1[i].volume == v2[i].volume);
        if (!valid) {
            printf("Differ at [%d]\n", i);
            printTrade(&v1[i]);
            printTrade(&v2[i]);
            return false;
        }
    }

    return true;
}

static void printTradeList(std::vector<Trade>& tv) {
    for (size_t i = 0; i < tv.size(); ++i) {
        printf("%ld:\t", i);
        printTrade(&tv[i]);
    }
}

}  // namespace ubiquant