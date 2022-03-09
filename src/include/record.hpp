#pragma once

struct Record {
    int order_id;
    double price;
    int volume;
};

struct BuyRecord : public Record {
    bool operator<(BuyRecord& br2) {
        if (price < br2.price)
            return true;
        return order_id < br2.order_id;
    };
};

struct SellRecord : public Record {
    bool operator<(SellRecord& sr2) {
        if (price > sr2.price)
            return true;
        return order_id < sr2.order_id;
    };
};

/* for debugging */
void printRecord(Record& r) {
    printf("order_id:%d\tprice:%.2f\tvolume:%d\n", r.order_id, r.price, r.volume);
}

void printRecordList(std::vector<Record>& rv) {
    for (auto& r: rv) {
        printRecord(r);
    }
}
