#include <cstring>
#include <iostream>
#include <string>

#include "loader.hpp"
#include "timer.hpp"

int main(int argc, char *argv[]) {
    int part_id;
    std::string cache_dir;

    if (argc < 2) {
        std::cout << "Usage: ./trader part_id [cache_dir]" << std::endl;
        return 0;
    } else if (argc == 2) {
        part_id = std::stoi(std::string(argv[1]));
    } else if (argc == 3) {
        part_id = std::stoi(std::string(argv[1]));
        cache_dir = std::string(argv[2]);
    } else {
        std::cout << "Usage: ./trader part_id [cache_dir]" << std::endl;
        return 0;
    }
    std::cout << "part_id: " << part_id << std::endl;
    std::cout << "cache_dir: " << cache_dir << std::endl;

    load_order_id_from_file(part_id);

    return 0;
}
