#pragma once

#include <string_view>
#include <string>

#include <userver/formats/json.hpp>

#include "chat/notifications/Notification.hpp"

namespace rl::handlers::notificaions::client
{
class SendMessageNotification
{
public:

public:
    std::string_view getName() const override
    {
        return "SendMessageNotification";
    }

    std::string toString() const override
    {

    }

    static std::string_view getStaticName()
    {
        return getName();
    }

private:
    std::string content_;
};
}