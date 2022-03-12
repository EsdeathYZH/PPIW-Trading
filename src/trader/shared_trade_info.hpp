#pragma once

#include <mutex>
#include <vector>

#include "common/config.hpp"
#include "common/type.hpp"

namespace ubiquant {

class SharedTradeInfo {
    std::mutex m;
    std::vector<uint64_t> sliding_window_start;

   public:
    void update_sliding_window_start(stock_code_t stock_code, uint64_t sliding_window_start) {
    }

    SharedTradeInfo() {
        sliding_window_start = std::vector<uint64_t>(Config::stock_num);
    }
};

}  // namespace ubiquant