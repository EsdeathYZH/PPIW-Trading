#pragma once

#include "debug.hpp"
#include "record.hpp"

class StockDeclarationBook {
/**
 * @brief 集中申报簿
 *
 * `buy_decls` 按照 price 降序（买价越高越优先）
 * `sell_decls` 按照 price 升序（卖价越低越优先）
 * 两者均可能为空，当两者均不为空时，买一价 严格小于 卖一价
 *
 * NOTE: 应当注意 price 的排序是不严格的，相邻两个 Record 的 price 可能相同
 *
 */
private:
    std::vector<BuyRecord> buy_decls;
    std::vector<SellRecord> sell_decls;
public:
    StockDeclarationBook () {};

    int insertBuyDecl(BuyRecord& br) {
        buy_decls.push_back(br);
        sort(buy_decls.begin(), buy_decls.end());
        return 0;
    }

    int insertSellDecl(SellRecord& sr) {
        sell_decls.push_back(sr);
        sort(sell_decls.begin(), sell_decls.end());
        return 0;
    }

    BuyRecord* queryBuyFirst() {
        if (buy_decls.empty())
            return nullptr;
        return &buy_decls[0];
    }

    void removeBuyFirst() {
        assert(!buy_decls.empty());
        buy_decls.erase(buy_decls.begin());
    }

    SellRecord* querySellFirst() {
        if (sell_decls.empty())
            return nullptr;
        return &sell_decls[0];
    }

    void removeSellFirst() {
        assert(!sell_decls.empty());
        sell_decls.erase(sell_decls.begin());
    }

    int totalBuyVolume() {
        int ret = 0;
        for (auto& br: buy_decls)
            ret += br.volume;
        return ret;
    }

    int totalSellVolume() {
        int ret = 0;
        for (auto& sr: sell_decls)
            ret += sr.volume;
        return ret;
    }
};