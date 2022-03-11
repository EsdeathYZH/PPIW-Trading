#pragma once

#include <memory>

#include "common/thread.h"
#include "network/msg_receiver.h"

namespace ubiquant {

class ExchangeOrderReceiver : public ubi_thread {
public:
    ExchangeOrderReceiver();

    void run() override;

protected:
    // socket server
    std::shared_ptr<MessageReceiver> msg_receiver_;
};

}  // namespace ubiquant
