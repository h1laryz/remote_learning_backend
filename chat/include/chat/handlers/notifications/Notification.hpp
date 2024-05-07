#pragma once

#include <string_view>

namespace rl::handlers::notificaions
{
class INotification
{
public:
    virtual std::string_view getName() const = 0;
    virtual std::string toString() const = 0;
};
}