#pragma once

#include <memory>

#include "common/type.hpp"
#include "common/global.hpp"
#include "common/thread.h"
#include "network/msg_receiver.h"

namespace ubiquant {

class ExchangeOrderReceiver : public ubi_thread {
public:
    ExchangeOrderReceiver();

    void run() override;

    void stop();
    void restart();
    void reset_network();

protected:
    // socket server
    std::shared_ptr<MessageReceiver> msg_receiver_;

    // receiver pause lock
    pthread_spinlock_t recv_lock;
};

}  // namespace ubiquant
