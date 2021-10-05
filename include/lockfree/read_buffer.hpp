#pragma once
#include "lockfree/index_pool.hpp"

#include <cstring>
#include <atomic>
#include <optional>

namespace broken
{

template <typename T>
class ReadBuffer
{
private:
    using storage_t = typename std::aligned_storage<sizeof(T), alignof(T)>::type;
    static constexpr uint32_t NUM_SLOTS = 16; //has influence on maximum concurrent writers that can be successful

    // our simple memory manager
    lockfree::IndexPool<NUM_SLOTS> m_indexpool; // contains indices from 0 to NUM_SLOTS-1
    storage_t m_storage[NUM_SLOTS];

    static constexpr uint32_t NO_DATA_INDEX = lockfree::IndexPool<NUM_SLOTS>::NO_INDEX;
    using index_t = lockfree::IndexPool<NUM_SLOTS>::index_t;

    static_assert(std::is_trivially_copyable<T>::value);
    static_assert(std::atomic<index_t>::is_always_lock_free);

public:
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

    std::optional<T> read()
    {
        auto index = m_dataIndex.load();

        while (index != NO_DATA_INDEX)
        {
            T *p = ptr(index);
            storage_t value;

            //ideally we could copy into the optional (check whether it is possible with STL)
            std::memcpy(&value, p, sizeof(T));

            if (m_dataIndex.compare_exchange_strong(index, index))
            {
                // nothing changed (... except ABA)
                return std::optional(*reinterpret_cast<T *>(&value));
            }
            // index changed during read
            // retry (index is loaded implicitly)
        }

        return std::nullopt;
    }

private:
    std::atomic<index_t>
        m_dataIndex{NO_DATA_INDEX};

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

} // namespace broken

namespace lockfree
{
template <typename T>
class ReadBuffer
{
private:
    using storage_t = typename std::aligned_storage<sizeof(T), alignof(T)>::type;
    static constexpr uint32_t NUM_SLOTS = 16;

    // our simple memory manager
    lockfree::IndexPool<NUM_SLOTS> m_indexpool; // contains indices from 0 to NUM_SLOTS-1
    storage_t m_storage[NUM_SLOTS];

    static constexpr uint32_t NO_DATA_INDEX = lockfree::IndexPool<NUM_SLOTS>::NO_INDEX;
    using index_t = lockfree::IndexPool<NUM_SLOTS>::index_t;

    struct AugmentedIndex
    {
        AugmentedIndex(index_t index = 0) : index(index)
        {
        }

        index_t index;
        uint32_t counter{0};

        operator index_t()
        {
            return index;
        }
    };

    static_assert(std::is_trivially_copyable<T>::value);
    static_assert(std::atomic<AugmentedIndex>::is_always_lock_free);

public:
    bool write(const T &data)
    {
        auto index = m_indexpool.get_index();
        if (index == NO_DATA_INDEX)
        {
            return false; // no memory
        }

        copy_to_slot(index, data);

        auto oldIndex = m_dataIndex.load();
        AugmentedIndex newIndex(index);
        do
        {
            newIndex.counter = oldIndex.counter + 1;
        } while (!m_dataIndex.compare_exchange_strong(oldIndex, newIndex));

        if (oldIndex != NO_DATA_INDEX)
        {
            free_slot(oldIndex);
        }

        return true;
    }

    bool try_write(const T &data)
    {
        auto index = m_indexpool.get_index();
        if (index == NO_DATA_INDEX)
        {
            return false; // no memory
        }

        copy_to_slot(index, data);

        auto oldIndex = m_dataIndex.load();
        AugmentedIndex newIndex(index);
        do
        {
            if (oldIndex != NO_DATA_INDEX)
            {
                free_slot(index);
                return false;
            }

            newIndex.counter = oldIndex.counter + 1;
        } while (!m_dataIndex.compare_exchange_strong(oldIndex, newIndex));

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

    std::optional<T> read()
    {
        auto index = m_dataIndex.load();

        while (index != NO_DATA_INDEX)
        {
            T *p = ptr(index);
            storage_t value;

            //ideally we could copy into the optional (check whether it is possible with STL)
            std::memcpy(&value, p, sizeof(T));

            if (m_dataIndex.compare_exchange_strong(index, index))
            {
                // nothing changed (... except ABA)
                return std::optional(*reinterpret_cast<T *>(&value));
            }
            // index changed during read
            // retry (index is loaded implicitly)
        }

        return std::nullopt;
    }

private:
    std::atomic<AugmentedIndex> m_dataIndex{NO_DATA_INDEX};

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
