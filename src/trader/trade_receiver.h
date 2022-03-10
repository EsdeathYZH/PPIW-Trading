#pragma once

#include <memory>

#include "common/thread.h"
#include "network/msg_receiver.h"

namespace ubiquant {

class TraderTradeReceiver : public ubi_thread {
public:
    TraderTradeReceiver();

    void run() override;

protected:
    // socket server
    std::shared_ptr<MessageReceiver> msg_receiver_;
};

}  // namespace ubiquant
