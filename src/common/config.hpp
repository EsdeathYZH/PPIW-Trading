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

    static int data_port_base __attribute__((weak));
    static int ctrl_port_base __attribute__((weak));
    static int server_port_base __attribute__((weak));

    static int sliding_window_size __attribute__((weak));
    static int stock_num __attribute__((weak));
};

// config init

std::string Config::data_folder;

int Config::data_port_base = 5500;
int Config::ctrl_port_base = 9576;
int Config::server_port_base = 6576;

int Config::sliding_window_size = 100;
int Config::stock_num = 10;

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
    } else if (cfg_name == "data_port_base") {
        Config::data_port_base = atoi(value.c_str());
        ASSERT(Config::data_port_base > 0);
    } else if (cfg_name == "ctrl_port_base") {
        Config::ctrl_port_base = atoi(value.c_str());
        ASSERT(Config::ctrl_port_base > 0);
    } else if (cfg_name == "server_port_base") {
        Config::server_port_base = atoi(value.c_str());
        ASSERT(Config::server_port_base > 0);
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
 * load config
 */
static void load_config(std::string fname, int nsrvs)
{
    ASSERT(nsrvs > 0);

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

    return;
}

/**
 * print current config
 */
static void print_config(void)
{
    std::cout << "------ configurations ------" << LOG_endl;

    // setting by config file
    std::cout << "data_folder: "          << Config::data_folder          << LOG_endl;
    std::cout << "data_port_base: "        << Config::data_port_base        << LOG_endl;
    std::cout << "ctrl_port_base: "        << Config::ctrl_port_base        << LOG_endl;
    std::cout << "server_port_base: "      << Config::server_port_base      << LOG_endl;
    std::cout << "sliding_window_size: "      << Config::sliding_window_size      << LOG_endl;
    std::cout << "--" << LOG_endl;
}

} // namespace wukong