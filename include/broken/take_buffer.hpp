#pragma once

#include <atomic>
#include <optional>
#include <type_traits>

namespace broken
{

template <typename T>
class TakeBuffer
{
public:
    bool write(const T &data)
    {
        auto newData = new (std::nothrow) T(data);
        if (!newData)
        {
            return false; // no memory
        }
        auto oldData = m_data.exchange(newData);
        delete oldData;
        return true;
    }

    bool try_write(const T &data)
    {
        auto newData = new (std::nothrow) T(data); // not necessarily lock-free!
        if (!newData)
        {
            return false; // no memory
        }

        T *expected = nullptr;
        if (m_data.compare_exchange_strong(expected, newData))
        {
            return true;
        }

        // buffer not empty

        delete newData;
        return true;
    }

    // could return a pointer, but then the user is responsible for deletion
    // (more efficient but also error-prone)
    std::optional<T> take()
    {
        auto data = m_data.exchange(nullptr);
        if (data)
        {
            std::optional ret(std::move(*data));
            delete data; // not necessarily lock-free!
            return ret;
        }

        return std::nullopt;
    }

private:
    std::atomic<T *> m_data{nullptr};
};

} // namespace broken
