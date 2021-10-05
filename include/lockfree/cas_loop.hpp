#pragma once

#include <atomic>
namespace lockfree
{

template <typename T>
bool compare_exchange_if_not_equal(std::atomic<T> &location, const T &expected, const T &newValue)
{
    auto value = location.load();

    while (value != expected)
    {
        if (location.compare_exchange_strong(value, newValue))
        {
            // we exchanged since the value did not match the expected one
            return true;
        }
        // value contains the updated value
    }

    // value matched expected, do not exchange
    return false;
}

// can create other atomic operations like this (fetch_multiply etc.)

} // namespace lockfree
