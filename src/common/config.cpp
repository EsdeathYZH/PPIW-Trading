#include "config.h"

namespace ubiquant {

// config init
std::string Config::data_folder;
std::string Config::network_config_file = "network.host";

int Config::sliding_window_size = 100;
int Config::stock_num = 10;
int Config::trader_num = 2;
int Config::exchange_num = 2;
int Config::partition_idx = -1;

std::vector<std::pair<int, int>> Config::trader0_exchange0;
std::vector<std::pair<int, int>> Config::trader0_exchange1;
std::vector<std::pair<int, int>> Config::trader1_exchange0;
std::vector<std::pair<int, int>> Config::trader1_exchange1;

std::string Config::trader0_addr;
std::string Config::trader1_addr;
std::string Config::exchange0_addr;
std::string Config::exchange1_addr;

} // namespace wukong