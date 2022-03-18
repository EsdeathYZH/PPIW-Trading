#include <algorithm>
#include <vector>
#include <cassert>
#include <cstdio>
#include <iostream>
#include <cstring>
#include <string>
#include <mutex>
#include <signal.h>

#include "common/global.hpp"
#include "common/config.h"
#include "common/console.hpp"

#include "trader/trader_controller.h"

#include "utils/timer.hpp"
#include "utils/util.h"

namespace ubiquant {

volatile bool work_flag = true;

inline bool at_work() { return work_flag; }

inline void finish_work() { work_flag = false; }

void stop_network() {
  Global<TraderController>::Get()->stop();
}

void restart_network() {
  Global<TraderController>::Get()->restart();
}

void reset_network() {
  Global<TraderController>::Get()->reset_network();
}

}

namespace {

std::mutex exit_lock;  // race between two signals
  
void printTraceExit(int sig) {
  ubiquant::print_stacktrace();
}

void sigsegv_handler(int sig) {
  std::lock_guard<std::mutex> lock(exit_lock);
  fprintf(stderr, "[Trader] Meet a segmentation fault!\n");
  // printTraceExit(sig);
  ubiquant::work_flag = false;
  exit(-1);
}

void sigint_handler(int sig) {
  std::lock_guard<std::mutex> lock(exit_lock);
  fprintf(stderr, "[Trader] Meet an interrupt!\n");
  // printTraceExit(sig);
  ubiquant::work_flag = false;
  exit(-1);
}

void sigabrt_handler(int sig) {
  std::lock_guard<std::mutex> lock(exit_lock);
  fprintf(stderr, "[Trader] Meet an assertion failure!\n");
  printTraceExit(sig);
  exit(-1);
}

} // anonymous namespace

using namespace ubiquant;

int main(int argc, char *argv[]) {
    /* install the event handler if necessary */
    signal(SIGSEGV, sigsegv_handler);
    signal(SIGABRT, sigabrt_handler);
    signal(SIGINT,  sigint_handler);

    /* parse command arguments */
    std::string config_file;
    if (argc != 4) {
        std::cout << "Usage: ./trader partition_idx config_file load_mode" << std::endl;
        return 0;
    } else {
        Config::partition_idx = std::stoi(std::string(argv[1]));
        Config::load_mode = std::stoi(std::string(argv[3]));
        ASSERT_MSG(Config::partition_idx == 0 
                || Config::partition_idx == 1, 
                    "partition_idx can only be 0 or 1!");
        config_file = std::string(argv[2]);
    }

    /* load config file */
    load_config(config_file);

    std::cout << "Trader[" << Config::partition_idx << "] is starting..." << std::endl;
    print_config();


    Global<TraderController>::New();

    Global<TraderController>::Get()->start();

    run_console("Trader["+std::to_string(Config::partition_idx)+"]");

    return 0;
}
