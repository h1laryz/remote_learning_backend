#pragma once

#include <cstdint>

#include <magic_enum_all.hpp>

namespace rl::handlers::errors
{
enum class Error : size_t
{
    kRequiredFieldIsEmpty,
    kRequiredFieldIsNotPassed,
};
}