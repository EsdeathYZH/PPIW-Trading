#pragma once

struct Record {
    int order_id;
    double price;
    int volume;
};

struct BuyRecord : public Record {
    bool operator<(BuyRecord& br2) {
        if (price == br2.price)
            return order_id > br2.order_id;
        return price < br2.price;
    };
};

struct SellRecord : public Record {
    bool operator<(SellRecord& sr2) {
        if (price == sr2.price)
            return order_id > sr2.order_id;
        return price > sr2.price;
    };
};

/* for debugging */
static void printRecord(Record& r) {
    printf("order_id:%d\tprice:%.2f\tvolume:%d\n", r.order_id, r.price, r.volume);
}

template<typename T>
void printRecordList(std::vector<T>& rv) {
    for (int i = 0; i < rv.size(); ++i) {
        printf("<%d>\t", i);
        printRecord(rv[i]);
    }
}
