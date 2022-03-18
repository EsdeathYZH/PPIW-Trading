#pragma once

#include <memory>

#include "common/global.hpp"
#include "common/block_queue.hpp"
#include "common/thread.h"
#include "common/type.hpp"
#include "network/msg_sender.h"

namespace ubiquant {

class TraderOrderSender : public ubi_thread {
public:
    TraderOrderSender(int exchange_idx);

    void run() override;

    void put_order(Order& order);

    void stop();
    void restart();
    void reset_network();

protected:
    int exchange_idx_;
    
    // socket client
    std::shared_ptr<MessageSender> msg_sender_;

    // order queue
    BlockQueue<Order> order_queue_;

    // sender pause lock
    pthread_spinlock_t send_lock;
};

}  // namespace ubiquant
