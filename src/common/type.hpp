#pragma once
#include <cstdint>

using stock_code_t = int;
using order_id_t = int;
using direction_t = int;
using type_t = int;
using price_t = double;
using volume_t = int;
using trade_idx_t = int;

// stk_code, order id and trade index start at 1...

struct order {
    int stk_code;
    int order_id;
    int direction;
    int type;
    double price;
    int volume;
};

struct trade {
    int stk_code;
    int bid_id;
    int ask_id;
    double price;
    int volume;
} __attribute__((packed));

class Coordinates {
   public:
    uint32_t coordinates;

    int get_x() {
        return (coordinates >> 20) & 0x3ff;
    }

    int get_y() {
        return (coordinates >> 10) & 0x3ff;
    }

    int get_z() {
        return coordinates & 0x3ff;
    }

    void set(int x, int y, int z) {
        coordinates = ((x & 0x3ff) << 20) | ((y & 0x3ff) << 10) | (z & 0x3ff);
    }
};

struct SortStruct {
    int order_id;
    Coordinates coor;

    bool operator<(const SortStruct rhs) {
        if (order_id != rhs.order_id)
            return order_id < rhs.order_id;
        else
            return coor.coordinates < rhs.coor.coordinates;
    }
};

struct HookTarget {
    int target_stk_code;
    int target_trade_idx;
    int arg;
};