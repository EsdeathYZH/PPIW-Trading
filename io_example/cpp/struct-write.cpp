#include <iostream>
#include <fstream>
using namespace std;

struct trade {
    int stk_code;
    int bid_id;
    int ask_id;
    double price;
    int volume;
}__attribute__((packed));

int main() {
    struct trade* t = new struct trade[2];
    t[0] = {1,2,3,4,5};
    t[1] = {5,4,3,2,1};
    std::ofstream outfile("Trade", std::ios::out | std::ios::binary);
    outfile.write((char *)t, sizeof(trade) * 2);
    outfile.close();
    return 0;
}
