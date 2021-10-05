#pragma once

#include <atomic>
#include <optional>
#include <iostream>
#include <type_traits>

#include "lockfree/index_pool.hpp"

namespace lockfree
{

template <typename T, uint32_t NUM_WRITERS = 16>
class TakeBuffer
{
private:
    using storage_t = typename std::aligned_storage<sizeof(T), alignof(T)>::type;
    static constexpr uint32_t NUM_SLOTS = NUM_WRITERS + 1;

    // our simple memory manager
    IndexPool<NUM_SLOTS> m_indexpool; // contains indices from 0 to NUM_SLOTS-1
    storage_t m_storage[NUM_SLOTS];

    static constexpr uint32_t NO_DATA_INDEX = IndexPool<NUM_SLOTS>::NO_INDEX;
    using index_t = typename IndexPool<NUM_SLOTS>::index_t;

public:
    // fails if out of memory, discards evicted element
    bool write(const T &data)
    {
        auto index = m_indexpool.get_index();
        if (index == NO_DATA_INDEX)
        {
            return false; // no memory
        }

        copy_to_slot(index, data);

        auto oldIndex = m_dataIndex.exchange(index);

        if (oldIndex != NO_DATA_INDEX)
        {
            free_slot(oldIndex);
        }

        return true;
    }

    // always succeeds
    std::optional<T> write_overflow(const T &data)
    {
        index_t index;

        do
        {
            index = m_indexpool.get_index();
        } while (index == NO_DATA_INDEX);

        copy_to_slot(index, data);

        auto oldIndex = m_dataIndex.exchange(index);

        if (oldIndex != NO_DATA_INDEX)
        {
            T *p = ptr(oldIndex);
            std::optional ret(std::move(*p));
            free_slot(oldIndex);
            return ret;
        }

        return std::nullopt;
    }

    // fails if buffer contains an element
    bool try_write(const T &data)
    {
        auto index = m_indexpool.get_index();
        if (index == NO_DATA_INDEX)
        {
            return false; // no memory
        }

        copy_to_slot(index, data);

        uint32_t expected = NO_DATA_INDEX;
        if (!m_dataIndex.compare_exchange_strong(expected, index))
        {
            free_slot(index);
            return false;
        }

        return true;
    }

    std::optional<T> take()
    {
        auto index = m_dataIndex.exchange(NO_DATA_INDEX);
        if (index == NO_DATA_INDEX)
        {
            return std::nullopt;
        }

        T *p = ptr(index);
        std::optional ret(std::move(*p));
        free_slot(index);
        return ret;
    }

private:
    std::atomic<index_t> m_dataIndex{NO_DATA_INDEX};

    T *ptr(index_t index)
    {
        return reinterpret_cast<T *>(&m_storage[index]);
    }

    void copy_to_slot(index_t index, const T &data)
    {
        T *p = ptr(index);
        new (p) T(data);
    }

    void free_slot(index_t index)
    {
        ptr(index)->~T();
        m_indexpool.return_index(index);
    }
};

} // namespace lockfree
