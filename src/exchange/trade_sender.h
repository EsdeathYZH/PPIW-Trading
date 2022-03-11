#pragma once

#include <memory>

#include "common/block_queue.hpp"
#include "common/thread.h"
#include "common/type.hpp"
#include "network/msg_sender.h"

namespace ubiquant {

class ExchangeTradeSender : public ubi_thread {
public:
    ExchangeTradeSender();

    void run() override;

    void put_trade(Trade& trade);

    // void put_order_ack(OrderAck& ack);

protected:
    // socket client
    std::shared_ptr<MessageSender> msg_sender_;

    // msg queue
    BlockQueue<std::string> msg_queue_;
};

}  // namespace ubiquant
