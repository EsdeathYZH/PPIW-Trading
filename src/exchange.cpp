#include <algorithm>
#include <vector>
#include <cassert>
#include <cstdio>
#include <iostream>
#include <string>
#include <mutex>
#include <signal.h>

#include "common/console.hpp"

#include "exchange/order_receiver.h"
#include "exchange/trade_sender.h"
#include "exchange/exchange.h"

#include "utils/timer.hpp"
#include "utils/util.h"

namespace ubiquant {

volatile bool work_flag = true;

inline bool at_work() { return work_flag; }

inline void finish_work() { work_flag = false; }

}

namespace {

std::mutex exit_lock;  // race between two signals
  
void printTraceExit(int sig) {
  ubiquant::print_stacktrace();
}

void sigsegv_handler(int sig) {
  std::lock_guard<std::mutex> lock(exit_lock);
  fprintf(stderr, "[Exchange] Meet a segmentation fault!\n");
  // printTraceExit(sig);
  ubiquant::work_flag = false;
  exit(-1);
}

void sigint_handler(int sig) {
  std::lock_guard<std::mutex> lock(exit_lock);
  fprintf(stderr, "[Exchange] Meet an interrupt!\n");
  // printTraceExit(sig);
  ubiquant::work_flag = false;
  exit(-1);
}

void sigabrt_handler(int sig) {
  std::lock_guard<std::mutex> lock(exit_lock);
  fprintf(stderr, "[Exchange] Meet an assertion failure!\n");
  printTraceExit(sig);
  exit(-1);
}

} // anonymous namespace

using namespace ubiquant;

int main(int argc, char *argv[])
{
    /* install the event handler if necessary */
    signal(SIGSEGV, sigsegv_handler);
    signal(SIGABRT, sigabrt_handler);
    signal(SIGINT,  sigint_handler);

    /* parse command arguments */
    std::string config_file;
    if (argc != 3) {
        std::cout << "Usage: ./exchange partition_idx config_file" << std::endl;
        return 0;
    } else {
        Config::partition_idx = std::stoi(std::string(argv[1]));
        ASSERT_MSG(Config::partition_idx == 0 
                || Config::partition_idx == 1, 
                    "partition_idx can only be 0 or 1!");
        config_file = std::string(argv[2]);
    }

    /* load config file */
    load_config(config_file);

    std::cout << "Exchange[" << Config::partition_idx << "] is starting..." << std::endl;
    print_config();

    Global<ExchangeOrderReceiver>::New();
    Global<ExchangeTradeSender>::New();
    Global<Exchange>::New();

    Global<ExchangeTradeSender>::Get()->start();
    Global<ExchangeOrderReceiver>::Get()->start();
    Global<Exchange>::Get()->start();

    run_console();

    return 0;
}