#pragma once
#include <cstdint>
#include <cstdio>
#include <memory>

#include "common/config.h"

namespace ubiquant {

using stock_code_t = int;
using order_id_t = int;
using direction_t = int;
using type_t = int;
using price_t = double;
using volume_t = int;
using trade_idx_t = int;

enum MSG_TYPE { ORDER_MSG = 1, TRADE_MSG = 2, ORDER_ACK_MSG = 3 };

template <typename T>
void get_elem_from_buf(const char* buf, size_t& offset, T& elem) {
    elem = *((T*)(buf + offset));
    offset += sizeof(T);
}

// stk_code, order id and trade index start at 1...

struct Order {
    int stk_code;
    int order_id;
    int direction;
    int type;
    double price;
    int volume;

    void append_to_str(std::string& str) const {
        str.reserve(str.length() + sizeof(Order));
        str.append((char*)this, sizeof(Order));
    }

    void from_str(std::string& str, size_t& offset) {
        const char* buf = (str.c_str() + offset);
        get_elem_from_buf(buf, offset, *this);
    }

    // Order() = default;

    // Order(const std::string& str) {
    //     from_str(str);
    // }

    void print() const {
        printf("[%d] order_id: %d\tdirection: %d\ttype: %d\tprice: %.2f\t, volume: %d\n",
               stk_code,
               order_id,
               direction,
               type,
               price,
               volume);
    }
};

struct Trade {
    int stk_code;
    int bid_id;
    int ask_id;
    double price;
    int volume;

    void append_to_str(std::string& str) const {
        str.reserve(str.length() + sizeof(Trade));
        str.append((char*)this, sizeof(Trade));
    }

    void from_str(std::string& str, size_t& offset) {
        const char* buf = (str.c_str() + offset);
        get_elem_from_buf(buf, offset, *this);
    }

    // Trade() = default;

    // Trade(const std::string& str) {
    //     from_str(str);
    // }

} __attribute__((packed));

struct OrderAck {
    int stk_code;
    int order_id;

    void append_to_str(std::string& str) const {
        str.reserve(str.length() + sizeof(OrderAck));
        str.append((char*)this, sizeof(OrderAck));
    }

    void from_str(std::string& str, size_t& offset) {
        const char* buf = (str.c_str() + offset);
        get_elem_from_buf(buf, offset, *this);
    }
} __attribute__((packed));

class Coordinates {
   public:
    uint32_t coordinates;

    int get_x() const {
        return (coordinates >> 20) & 0x3ff;
    }

    int get_y() const {
        return (coordinates >> 10) & 0x3ff;
    }

    int get_z() const {
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

class OrderInfoMatrix {
   public:
    std::shared_ptr<direction_t[]> direction_matrix;
    std::shared_ptr<type_t[]> type_matrix;
    std::shared_ptr<price_t[]> price_matrix;
    std::shared_ptr<volume_t[]> volume_matrix;

    Order generate_order(const stock_code_t stock_code, const SortStruct ss, const int nx, const int ny, const int nz) const {
        int x = ss.coor.get_x(), y = ss.coor.get_y(), z = ss.coor.get_z();
        assert((0 <= x && x < nx) && (0 <= y && y < ny) && (0 <= z && z < nz));
        assert(stock_code == x % Config::stock_num + 1);
        Order order;
        order.stk_code = stock_code;
        order.order_id = ss.order_id;
        order.direction = direction_matrix[x * (ny * nz) + y * (nz) + z];
        order.type = type_matrix[x * (ny * nz) + y * (nz) + z];
        order.price = price_matrix[x * (ny * nz) + y * (nz) + z];
        order.volume = volume_matrix[x * (ny * nz) + y * (nz) + z];

        return order;
    }
};

}  // namespace ubiquant
