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

    void put_order_ack(OrderAck& ack);

    volatile bool sender_running = true;

protected:
    // socket client
    std::vector<std::shared_ptr<MessageSender>> msg_senders_;

    // msg queue
    BlockQueue<std::string> msg_queue_;
};

}  // namespace ubiquant
