#pragma once

#include <sys/time.h>
#include <stdint.h>

#include "util.h"

namespace ubiquant {

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


class Breakdown_Timer {
  
 public:
  Breakdown_Timer()
    : sum(0), count(0) { 
    buffer.reserve(max_elems_);  
  }

  inline void start() { 
    temp = rdtsc();
  }

  inline float get_diff_ms() const {
    float res = rdtsc() - temp;
    return (res / get_one_second_cycle()) * 1000.0;
  }

  inline void end() {
    uint64_t t = rdtsc(); 
    uint64_t res = (t - temp);
    sum += res; 
    ++count;
    if(buffer.size() >= max_elems_) return;
    buffer.push_back(res);
  }

  void emplace(uint64_t res) {
    sum += res;
    ++count;
    if(buffer.size() >= max_elems_) return;
    buffer.push_back(res);
  }

  inline double report() {
    if(count == 0) return 0.0; // avoids divided by zero
    double ret =  (double) sum / (double) count;
    // clear the info
    sum = 0;
    count = 0;
    // buffer.clear();  // can not used here
    return ret;
  }

  void calculate_detailed() {
    if(buffer.size() == 0) return;

    // first erase some items
    int idx = std::floor(buffer.size() * 0.1 / 100.0);
    buffer.erase(buffer.begin(), buffer.begin() + idx + 1);

    // then sort
    std::sort(buffer.begin(),buffer.end());
    int num = std::floor(buffer.size() * 0.01 / 100.0);
    buffer.erase(buffer.begin() + buffer.size() - num, buffer.begin() + buffer.size());
  }

  inline double report_medium() const {
    if(buffer.size() == 0) return 0;
    // for (int i = 0; i < buffer.size(); ++i)
    //   printf("buffer[%d] = %lf\n", i, (double) buffer[i]);
    return buffer[buffer.size() / 2];
  }

  inline double report_90() const {
    if(buffer.size() == 0) return 0;
    int idx = std::floor( buffer.size() * 90 / 100.0);
    return buffer[idx];
  }

  inline double report_99() const {
    if(buffer.size() == 0) return 0;
    int idx = std::floor(buffer.size() * 99 / 100.0);
    return buffer[idx];
  }

  inline static uint64_t get_one_second_cycle() {
    static uint64_t one_sec_cycle = 0;
    if (one_sec_cycle == 0) {
      uint64_t begin = rdtsc();
      sleep(1);
      one_sec_cycle = rdtsc() - begin;
    }
    return one_sec_cycle;
  }

  inline size_t size() const { return buffer.size(); }

 private:
  static const uint64_t max_elems_ = 1000000;

  uint64_t sum;
  uint64_t count;
  uint64_t temp;
  std::vector<uint64_t> buffer;
};

}  // namespace ubiquant
