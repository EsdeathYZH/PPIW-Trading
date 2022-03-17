#pragma once

#include <string>

#include "common/type.hpp"
#include "utils/timer.hpp"

namespace ubiquant {

class Monitor {
   private:
    bool isrunning = false;
    uint64_t cnt = 0;
    uint64_t start_time = 0;
    uint64_t end_time = 0;

    uint64_t last_separator = 0;
    uint64_t last_time = 0;
    uint64_t last_cnt = 0;

    uint64_t interval = 1000 * 1000;

   public:
    void start_thpt() {
        isrunning = true;
        start_time = timer::get_usec();
        last_separator = start_time;
        last_time = start_time;
    }

    void end_thpt() {
        isrunning = false;
        end_time = timer::get_usec();
    }

    void add_cnt() {
        assert(isrunning);
        cnt++;
    }

    // print the throughput of a fixed interval
    inline void print_timely_thpt(const std::string& prefix = "") {
        uint64_t now = timer::get_usec();

        // print separators per second
        if (now - last_separator > interval) {
            logstream(LOG_INFO) << prefix << " [" << (now - start_time) / interval << "sec]" << LOG_endl;
            last_separator = now;
        }

        // periodically print timely throughput
        if ((now - last_time) > interval) {
            float cur_thpt = 1000000.0 * (cnt - last_cnt) / (now - last_time);
            logstream(LOG_INFO) << prefix << " Throughput: " << cur_thpt << " requests/sec" << LOG_endl;
            last_time = now;
            last_cnt = cnt;
        }
    }
};

};  // namespace ubiquant