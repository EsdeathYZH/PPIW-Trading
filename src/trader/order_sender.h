#pragma once

#include <memory>

#include "common/thread.h"
#include "network/msg_sender.h"

namespace ubiquant {

class TraderOrderSender : public ubi_thread {
public:
    TraderOrderSender();

    void run() override;

protected:
    // socket client
    std::shared_ptr<MessageSender> msg_sender_;
};

}  // namespace ubiquant
