#include <iostream>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/param.h>

#include <common/type.hpp>

namespace ubiquant {

std::vector<Trade> read_trade_from_file(std::string filename)
{
    int fd = open(filename.c_str(), O_RDWR);
    if (fd < 0) {
        std::cout << "Error while open " << filename << std::endl;
        return {};
    }

    char trade_buf[24];
    std::vector<Trade> ret;

    while (read(fd, trade_buf, 24) == 24) {
        Trade *tptr = reinterpret_cast<Trade*>(trade_buf);
        ret.push_back(*tptr);
    }

    close(fd);
    return ret;
}

bool operator!=(Trade& t1, Trade& t2)
{
    if (t1.stk_code != t2.stk_code)
        return true;
    if (t1.ask_id != t2.ask_id)
        return true;
    if (t1.bid_id != t2.bid_id)
        return true;
    // if (t1.price != t2.price)
    //     return true;
    // if (t1.volume != t2.volume)
    //     return true;
    return false;
}

} // namespace uniquant

using namespace ubiquant;

int main(int argc, char** argv)
{
    if (argc != 3) {
        std::cout << "usage: output_diff <answer> <our_output>" << std::endl;
        return 0;
    }

    std::vector<Trade> answer = read_trade_from_file(argv[1]);
    std::vector<Trade> output = read_trade_from_file(argv[2]);
    std::cout << "answer size: " << answer.size() << std::endl;
    std::cout << "output size: " << output.size() << std::endl;

    for (int i = 0; i < MIN(answer.size(), output.size()); ++i) {
        if (answer[i] != output[i]) {
            std::cout << "diff at [" << i << ']' << std::endl;
            for (int k = i - 10; k < i + 10; ++k) {
                answer[k].print();
                output[k].print();
                std::cout << std::endl;
            }
            return 0;
        }
    }

    return 0;
}