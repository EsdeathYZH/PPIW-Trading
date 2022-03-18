#pragma once

#include <iostream>
#include <string>

#include <boost/unordered_map.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>

#include "config.h"

#include "utils/errors.hpp"
#include "utils/logger2.hpp"

using namespace boost;
using namespace boost::program_options;

namespace ubiquant {

// options
options_description        all_desc("These are common exchange commands: ");
options_description       help_desc("help                display help infomation");
options_description       quit_desc("quit                quit from the console");
options_description     config_desc("config <args>       run commands for configueration");
options_description     logger_desc("logger <args>       run commands for logger");
options_description     sparql_desc("sparql <args>       run SPARQL queries in single or batch mode");

/*
 * init the options_description
 * it should add the option to different options_description
 * all should be added to all_desc
 */
void init_options_desc()
{
    // e.g., ubiquant> help
    all_desc.add(help_desc);

    // e.g., ubiquant> quit
    all_desc.add(quit_desc);

    // e.g., ubiquant> config <args>
    config_desc.add_options()
    (",v", "print current config")
    (",l", value<std::string>()->value_name("<fname>"), "load config items from <fname>")
    (",s", value<std::string>()->value_name("<string>"), "set config items by <str> (e.g., item1=val1&item2=...)")
    ("help,h", "help message about config")
    ;
    all_desc.add(config_desc);

    // e.g., ubiquant> logger <args>
    logger_desc.add_options()
    (",v", "print loglevel")
    (",s", value<int>()->value_name("<level>"), "set loglevel to <level> (e.g., DEBUG=1, INFO=2, WARNING=4, ERROR=5)")
    ("help,h", "help message about logger")
    ;
    all_desc.add(logger_desc);

    // e.g., ubiquant> sparql <args>
    sparql_desc.add_options()
    (",f", value<std::string>()->value_name("<fname>"), "run a [single] SPARQL query from <fname>")
    (",m", value<int>()->default_value(1)->value_name("<factor>"), "set multi-threading <factor> for heavy query processing")
    (",p", value<std::string>()->value_name("<fname>"), "adopt user-defined query plan from <fname> for running a single query")
    (",N", value<int>()->default_value(1)->value_name("<num>"), "do query optimization <num> times")
    (",o", value<std::string>()->value_name("<fname>"), "output results into <fname>")
    ("help,h", "help message about sparql")
    ;
    all_desc.add(sparql_desc);
}


/**
 * fail to parse the command
 */
static void fail_to_parse(int argc, char** argv)
{
    std::string cmd;
    for (int i = 0; i < argc; i++)
        cmd = cmd + argv[i] + " ";

    logstream(LOG_ERROR) << " Failed to run the command: " << cmd << LOG_endl;
    std::cout << std::endl << "Input \'help\' command to get more information." << std::endl;
}

/**
 * split the string into char** by the space
 * the argc is the name of char*
 * the return value argv of get_args is the type of char**
 */
class CMD2Args {
private:
    char** cmd_argv; /// the cmd argument variables
    int cmd_argc; /// the number of cmd arguments

public:
    CMD2Args(std::string str)
    {
        std::vector<std::string> fields;

        split(fields, str, is_any_of(" "));

        cmd_argc = fields.size(); // set the argc as number of arguments
        cmd_argv = new char*[cmd_argc + 1];
        for (int i = 0; i < cmd_argc; i++) {
            cmd_argv[i] = new char[fields[i].length() + 1];
            strcpy(cmd_argv[i], fields[i].c_str());
        }
        cmd_argv[cmd_argc] = NULL;
    }

    char** get_args(int &argc)
    {
        argc = cmd_argc;
        return cmd_argv;
    }

    ~CMD2Args()
    {
        for (int i = 0; i < cmd_argc; ++i)
        {
            delete cmd_argv[i];
        }
        delete []cmd_argv;
    }
};

static void file2str(std::string fname, std::string &str)
{
    std::ifstream file(fname.c_str());
    if (!file) {
        logstream(LOG_ERROR) << fname << " does not exist." << LOG_endl;
        return;
    }

    std::string line;
    while (getline(file, line))
        str += line + " ";
}

static void args2str(std::string &str)
{
    size_t found = str.find_first_of("=&");
    while (found != std::string::npos) {
        str[found] = ' ';
        found = str.find_first_of("=&", found + 1);
    }
}


/**
 * print help of all commands
 */
static void print_help(void)
{
    std::cout << all_desc << std::endl;
}

/**
 * run the 'config' command
 * usage:
 * config [options]
 *   -v          print current config
 *   -l <fname>  load config items from <file>
 *   -s <str>    set config items by <str> (format: item1=val1&item2=...)
 */
static void run_config(int argc, char **argv)
{
    // parse command
    variables_map config_vm;
    try {
        store(parse_command_line(argc, argv, config_desc), config_vm);
    } catch (...) {
        fail_to_parse(argc, argv);
        return;
    }
    notify(config_vm);

    // parse options
    if (config_vm.count("help")) {
        std::cout << config_desc;
        return;
    }

    if (config_vm.count("-v")) {
        print_config();
        return;
    }

    // exclusive
    if (!(config_vm.count("-l") ^ config_vm.count("-s"))) {
        fail_to_parse(argc, argv); // invalid cmd
        return;
    }

    std::string fname, str;
    if (config_vm.count("-l")) {
        fname = config_vm["-l"].as<std::string>();
        file2str(fname, str);
    }

    if (config_vm.count("-s")) {
        str = config_vm["-s"].as<std::string>();
        args2str(str);
    }

    /// do config
    if (!str.empty()) {
        reload_config(str);
    } else {
        logstream(LOG_ERROR) << "Failed to load config file: " << fname << LOG_endl;
    }
}

/**
 * run the 'logger' command
 * usage:
 * logger [options]
 *   -v          print current log level
 *   -s <level>  set log level to <level> (i.e., EVERYTHING=0, DEBUG=1, INFO=2, EMPH=3,
 *                                               WARNING=4, ERROR=5, FATAL=6, NONE=7)
 */
static void run_logger(int argc, char **argv)
{
    // parse command
    variables_map logger_vm;
    try {
        store(parse_command_line(argc, argv, logger_desc), logger_vm);
    } catch (...) {
        fail_to_parse(argc, argv);
        return;
    }
    notify(logger_vm);

    // parse options
    if (logger_vm.count("help")) {
        std::cout << logger_desc;
        return;
    }

    if (logger_vm.count("-v")) {
        int level = global_logger().get_log_level();
        std::cout << "loglevel: " << level
                << " (" << levelname[level] << ")" << std::endl;
        return;
    }

    if (logger_vm.count("-s")) {
        int level = global_logger().get_log_level();  // orginal loglevel
        level = logger_vm["-s"].as<int>();

        if (level >= LOG_EVERYTHING && level <= LOG_NONE) {
            global_logger().set_log_level(level);
            std::cout << "set loglevel to " << level
                    << " (" << levelname[level] << ")" << std::endl;
        } else {
            logstream(LOG_ERROR) << "failed to set loglevel: " << level
                                    << " (" << levelname[level] << ")" << std::endl;
        }
        return;
    }
}

/**
 * run the 'sparql' command
 * usage:
 * sparql -f <fname> [options]
 *   -m <factor>  set multi-threading factor <factor> for heavy query processing
 *   -n <num>     run <num> times
 *   -p <fname>   adopt user-defined query plan from <fname> for running a single query
 *   -N <num>     do query optimization <num> times
 *   -v <lines>   print at most <lines> of results
 *   -o <fname>   output results into <fname>
 *   -g           leverage GPU to accelerate heavy query processing
 *
 * sparql -b <fname>
 */
static void run_sparql(int argc, char **argv)
{

    // parse command
    variables_map sparql_vm;
    try {
        store(parse_command_line(argc, argv, sparql_desc), sparql_vm);
    } catch (...) { // something go wrong
        fail_to_parse(argc, argv);
        return;
    }
    notify(sparql_vm);

    // parse options
    if (sparql_vm.count("help")) {
        std::cout << sparql_desc;
        return;
    }

    // single mode (-f) and batch mode (-b) are exclusive
    if (!(sparql_vm.count("-f") ^ sparql_vm.count("-b"))) {
        logstream(LOG_ERROR) << "single mode (-f) and batch mode (-b) are exclusive!" << LOG_endl;
        fail_to_parse(argc, argv); // invalid cmd
        return;
    }

    /// [single mode]
    if (sparql_vm.count("-f")) {
        std::string fname = sparql_vm["-f"].as<std::string>();
        std::ifstream query_stream(fname);
        if (!query_stream.good()) { // fail to load query file
            logstream(LOG_ERROR) << "The query file is not found: "
                                    << fname << LOG_endl;
            fail_to_parse(argc, argv); // invalid cmd
            return;
        }

        // NOTE: the options with default_value are always available.
        //       default value: mt_factor(1), cnt(1), nopts(1), nlines(0)

        // option: -m <factor>, -n <num>
        int mt_factor = sparql_vm["-m"].as<int>(); // the number of multithreading
        int cnt = sparql_vm["-n"].as<int>();     // the number of executions

        int nopts = sparql_vm["-N"].as<int>();

        // option: -v <lines>, -o <fname>
        int nlines = sparql_vm["-v"].as<int>();  // the number of result lines

        std::string ofname = "";
        if (sparql_vm.count("-o"))
            ofname = sparql_vm["-o"].as<std::string>();

        // option: -g
        bool snd2gpu = sparql_vm.count("-g");

        

        try {
            // TODO
        } catch (UbiquantException &ex) {
            logstream(LOG_ERROR) << "Query failed [ERRNO " << ex.code()
                                 << "]: " << ex.what() << LOG_endl;
            fail_to_parse(argc, argv);  // invalid cmd
            return;
        }
    }
}

void run_console(std::string console_name)
{
    std::cout << std::endl
            << "Input \'help\' command to get more information"
            << std::endl
            << std::endl;

    bool once = true;
    while (true) {
        std::string cmd;

        // interactive mode: print a prompt and retrieve the command
        // skip input with blank
        size_t pos;
        do {
            std::cout << console_name << "> ";
            std::getline(std::cin, cmd);
            pos = cmd.find_first_not_of(" \t");
        } while (pos == std::string::npos); // if all are blanks, do again

        // trim input
        pos = cmd.find_first_not_of(" \t"); // trim blanks from head
        cmd.erase(0, pos);
        pos = cmd.find_last_not_of(" \t");  // trim blanks from tail
        cmd.erase(pos + 1, cmd.length() - (pos + 1));

        // transform the comnmand to (argc, argv)
        CMD2Args cmd2args(cmd);
        int argc = 0;
        char **argv = cmd2args.get_args(argc);


        // run commmand on all proxies according to the keyword
        std::string cmd_type = argv[0];
        try {
            if (cmd_type == "help" || cmd_type == "h") {
                print_help();
            } else if (cmd_type == "quit" || cmd_type == "q") {
                exit(0);
            } else if (cmd_type == "config") {
                run_config(argc, argv);
            } else if (cmd_type == "logger") {
                run_logger(argc, argv);
            } else if (cmd_type == "sparql") {  // handle SPARQL queries
                run_sparql(argc, argv);
            } else {
                // the same invalid command dispatch to all proxies, print error msg once
                fail_to_parse(argc, argv);
            }
        } catch (UbiquantException &ex) {
            logstream(LOG_ERROR) << "ERRNO " << ex.code() << ": " << ex.what() << LOG_endl;
            fail_to_parse(argc, argv);
        }
    }
}

} // namespace ubiquant
