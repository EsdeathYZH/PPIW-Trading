#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <string>
#include <stdio.h>
#include <stdlib.h>

#include <boost/algorithm/string/predicate.hpp>

#include "utils/logger2.hpp"
#include "utils/assertion.hpp"

namespace ubiquant {

class Config {
public:
    // another choice
    // e.g., static int &num_threads() { static int _num_threads = 2; return _num_threads; }

    static std::string data_folder __attribute__((weak));
    static std::string network_config_file __attribute__((weak));

    static int partition_idx __attribute__((weak));
    static int sliding_window_size __attribute__((weak));
    static int stock_num __attribute__((weak));
    static int trader_num __attribute__((weak));
    static int exchange_num __attribute__((weak));

    static std::string trader0_addr;
    static std::string trader1_addr;
    static std::string exchange0_addr;
    static std::string exchange1_addr;
    static std::vector<std::pair<int, int>> trader0_exchange0;
    static std::vector<std::pair<int, int>> trader0_exchange1;
    static std::vector<std::pair<int, int>> trader1_exchange0;
    static std::vector<std::pair<int, int>> trader1_exchange1;
};

static bool set_immutable_config(std::string cfg_name, std::string value)
{
    if (cfg_name == "data_folder") {
        Config::data_folder = value;

        // make sure to check that the Config::data_folder is non-empty.
        if (Config::data_folder.length() == 0) {
            logstream(LOG_ERROR) << "the directory path of RDF data can not be empty!"
                                 << "You should set \"data_folder\" in config file." << LOG_endl;
            exit(-1);
        }

        // force a "/" at the end of Config::data_folder.
        if (Config::data_folder[Config::data_folder.length() - 1] != '/')
            Config::data_folder = Config::data_folder + "/";
    } else if (cfg_name == "network_config_file") {
        Config::network_config_file = value;
        // make sure to check that the Config::data_folder is non-empty.
        if (Config::network_config_file.length() == 0) {
            logstream(LOG_ERROR) << "the file name of network config can not be empty!"
                                 << "You should set \"network_config_file\" in config file." << LOG_endl;
            exit(-1);
        }
    } else {
        return false;
    }

    return true;
}

static bool set_mutable_config(std::string cfg_name, std::string value)
{
    if (cfg_name == "sliding_window_size") {
        Config::sliding_window_size = atoi(value.c_str());
    } else {
        return false;
    }

    return true;
}

static void str2items(std::string str, std::map<std::string, std::string> &items)
{
    std::istringstream iss(str);
    std::string row, val;
    while (iss >> row >> val)
        items[row] = val;
}

static void file2items(std::string fname, std::map<std::string, std::string> &items)
{
    std::ifstream file(fname.c_str());
    if (!file) {
        logstream(LOG_ERROR) << fname << " does not exist." << LOG_endl;
        exit(0);
    }

    std::string line, row, val;
    while (std::getline(file, line)) {
        if (boost::starts_with(line, "#") || line.empty())
            continue; // skip comments and blank lines

        std::istringstream iss(line);
        iss >> row >> val;
        items[row] = val;
    }
}

/**
 * reload config
 */
static void reload_config(std::string str)
{
    // TODO: it should ensure that there is no outstanding queries.

    // load config file
    std::map<std::string, std::string> items;
    str2items(str, items);

    for (auto const &entry : items)
        set_mutable_config(entry.first, entry.second);

    return;
}


/**
 * load network config
 */
static void load_network_config(std::string fname)
{
    // load network config file
    std::ifstream file(fname.c_str());
    if (!file) {
        logstream(LOG_ERROR) << fname << " does not exist." << LOG_endl;
        exit(0);
    }

    std::string line, row, val;
    while (std::getline(file, line)) {
        if (boost::starts_with(line, "#") || line.empty())
            continue; // skip comments and blank lines

        std::istringstream iss(line);
        iss >> row;
        if(row == "trader0") {
            iss >> Config::trader0_addr;
        } else if(row == "trader1") {
            iss >> Config::trader1_addr;
        } else if(row == "exchange0") {
            iss >> Config::exchange0_addr;
        } else if(row == "exchange1") {
            iss >> Config::exchange1_addr;
        } else if(row == "trader0:exchange0") {
            int trader_port, exchange_port;
            iss >> trader_port >> exchange_port;
            Config::trader0_exchange0.push_back({trader_port, exchange_port});
        } else if(row == "trader0:exchange1") {
            int trader_port, exchange_port;
            iss >> trader_port >> exchange_port;
            Config::trader0_exchange1.push_back({trader_port, exchange_port});
        } else if(row == "trader1:exchange0") {
            int trader_port, exchange_port;
            iss >> trader_port >> exchange_port;
            Config::trader1_exchange0.push_back({trader_port, exchange_port});
        } else if(row == "trader1:exchange1") {
            int trader_port, exchange_port;
            iss >> trader_port >> exchange_port;
            Config::trader1_exchange1.push_back({trader_port, exchange_port});
        } else {
            logstream(LOG_ERROR) << "Unsupport item:" << row << LOG_endl;
        }
    }
    

    return;
}

/**
 * load config
 */
static void load_config(std::string fname)
{
    // load config file
    std::map<std::string, std::string> items;
    file2items(fname, items);

    for (auto const &entry : items) {
        if (!(set_immutable_config(entry.first, entry.second)
                || set_mutable_config(entry.first, entry.second))) {
            logstream(LOG_WARNING) << "unsupported configuration item! ("
                                   << entry.first << ")" << LOG_endl;
        }
    }

    load_network_config(Config::data_folder + Config::network_config_file);

    return;
}

/**
 * print current config
 */
static void print_config(void)
{
    std::cout << "--------- CONFIG ---------" << LOG_endl;

    // print regular config setting by config file
    std::cout << "partition_idx: "        << Config::partition_idx  << LOG_endl;
    std::cout << "data_folder: "          << Config::data_folder          << LOG_endl;
    std::cout << "network_config_file: "  << Config::network_config_file  << LOG_endl;
    std::cout << "sliding_window_size: "  << Config::sliding_window_size  << LOG_endl;
    std::cout << "stock_num: "            << Config::stock_num  << LOG_endl;
    std::cout << "trader_num: "           << Config::trader_num  << LOG_endl;
    std::cout << "exchange_num: "         << Config::exchange_num  << LOG_endl;

    // print network config
    std::cout << "trader0_addr: "         << Config::trader0_addr  << LOG_endl;
    std::cout << "trader1_addr: "         << Config::trader0_addr  << LOG_endl;
    std::cout << "exchange0_addr: "       << Config::exchange0_addr  << LOG_endl;
    std::cout << "exchange1_addr: "       << Config::exchange1_addr  << LOG_endl;
    std::cout << "trader0->exchange0 port mapping: " << LOG_endl;
    for(auto& [trader_port, exchange_port] : Config::trader0_exchange0) {
        std::cout << trader_port << "->" << exchange_port << LOG_endl;
    }
    std::cout << "trader0->exchange1 port mapping: " << LOG_endl;
    for(auto& [trader_port, exchange_port] : Config::trader0_exchange1) {
        std::cout << trader_port << "->" << exchange_port << LOG_endl;
    }
    std::cout << "trader1->exchange0 port mapping: " << LOG_endl;
    for(auto& [trader_port, exchange_port] : Config::trader1_exchange0) {
        std::cout << trader_port << "->" << exchange_port << LOG_endl;
    }
    std::cout << "trader1->exchange1 port mapping: " << LOG_endl;
    for(auto& [trader_port, exchange_port] : Config::trader1_exchange1) {
        std::cout << trader_port << "->" << exchange_port << LOG_endl;
    }
    std::cout << "----------- END ------------" << LOG_endl;
}

} // namespace wukong