#include "config.h"

namespace ubiquant {

// config init
std::string Config::data_folder;
std::string Config::trade_output_folder;
std::string Config::network_config_file = "/home/team-7/yzh/awsome-10w/network.host";

int Config::sliding_window_size = 100;
int Config::stock_num = 10;
int Config::trader_num = 2;
int Config::exchange_num = 2;
int Config::partition_idx = -1;
int Config::loader_nx_matrix = 500;
int Config::loader_ny_matrix = 1000;
int Config::loader_nz_matrix = 1000;

// 0: don't have cache file, load and write cache file
// 1: have cache file
// 2: forget cache, all data in memory
int Config::load_mode = 0;

std::vector<std::vector<std::vector<std::pair<int, int>>>> Config::trader_port2exchange_port;

std::vector<std::string> Config::traders_addr;
std::vector<std::string> Config::exchanges_addr;

} // namespace wukong