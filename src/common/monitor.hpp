#pragma once

#include <queue>
#include <string>
#include <mutex>

#include "common/global.hpp"
#include "common/type.hpp"
#include "utils/timer.hpp"

namespace ubiquant {

class LogBuffer {
    mutable std::mutex m_mutex;

    std::vector<std::string> buf;
    const int max_length = 10;
    int start_idx = 0;
    int cur_size = 0;

   public:
    void add_log(const std::string& log) {
        std::lock_guard<std::mutex> guard(m_mutex);
        if (cur_size == max_length) {
            buf[start_idx] = log;
            start_idx = (start_idx + 1) % max_length;
        } else {
            buf[(start_idx + cur_size) % max_length] = log;
            cur_size++;
        }
    }

    void print_log() {
        std::lock_guard<std::mutex> guard(m_mutex);
        std::string log;
        int idx = start_idx;
        for (int i = 0; i < max_length; i++) {
            log += buf[idx];
            idx = (idx + 1) % max_length;
        }
        std::cout << log << std::endl;
    }

    LogBuffer() {
        buf.resize(max_length);
    }
};

class Monitor {
   private:
    bool isrunning = false;
    uint64_t cnt = 0;
    uint64_t start_time = 0;
    uint64_t end_time = 0;

    uint64_t last_separator = 0;
    uint64_t last_time = 0;
    uint64_t last_cnt = 0;

    uint64_t interval = 10 * 1000 * 1000;

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
            Global<LogBuffer>::Get()->add_log(prefix + " [" + std::to_string((now - start_time) / interval) + "sec]\n");
            last_separator = now;
        }

        // periodically print timely throughput
        if ((now - last_time) > interval) {
            float cur_thpt = 1000000.0 * (cnt - last_cnt) / (now - last_time);
            Global<LogBuffer>::Get()->add_log(prefix + " Throughput: " + std::to_string(cur_thpt) + " requests/sec\n");
            last_time = now;
            last_cnt = cnt;
        }
    }
};

};  // namespace ubiquant