#include "demo/d2_index_pool.hpp"

namespace broken
{

// 1) present interface we want to implement
// write, try_write, take

template <typename T>
class TakeBuffer
{
private:
    // LESSON: can only modify small data atomically with lockfree operations
    std::atomic<T *> m_data{nullptr};

public:
    bool write(const T &data)
    {
        // failure -> new is in general not be lockfree
        auto copy = new (std::no_throw) T(data);
        if (copy == nullptr)
        {
            return false; // no memory
        }

        // LESSON: prepare operation locally and finalize/make it observable with
        // a single atomic operation
        auto oldData = m_data.exchange(copy);

        if (oldData)
        {
            // not lockfree in general
            delete oldData;
        }

        return true;
    }

    bool try_write(const T &data)
    {
        // ignore here
        return false;
    }

    std::optional<T> take()
    {
        // todo
    }
};

// lesson: manage our own memory
// lesson transfer ownership between buffers
namespace lockfree
{

template <typename T, uint32_t C = 8>
class ReadBuffer
{
private:
    using storage_t = typename std::aligned_storage<sizeof(T), alignof(T)>::type;
    using indexpool_t = lockfree::index_pool<C>;
    // LESSON: prefer indices to pointers (better control, fewer bits needed, bits are precious in CAS)
    using indexpool_t::index_t;

    static constexpr index_t NO_DATA = C;

    std::atomic<index_t> m_value{NO_DATA};
    indexpool_t m_indices;
    storage_t m_buffer[C];

public:
    bool write(const T &value)
    {
        auto maybeIndex = m_indices.get_index();
        if (!maybeIndex)
        {
            return false; // no memory
        }
        auto index = maybeIndex.value();
        auto p = ptr(index);
        new (p) T(value);

        auto oldIndex = m_value.exchange(index);
        free(oldIndex);
        return true;
    }

    bool try_write(const T &value)
    {
        auto maybeIndex = m_indices.get_index();
        if (!maybeIndex)
        {
            return false; // no memory
        }
        auto index = maybeIndex.value();
        auto p = ptr(index);
        new (p) T(value);

        auto expectedIndex == NO_DATA;
        do
        {
            // LESSON: can use weak CAS in a loop
            // weak: may fail even if it contains NO_DATA ...
            if (m_index.compare_exchange_weak(expectedIndex, index))
            {
                return true;
            }
        } while (expectedIndex == NO_DATA);
        free(index);
        return false;
    }

    std::optional<T> take()
    {
        auto index = m_index.exchange(NO_DATA);
        if (index == NO_DATA)
        {
            return std::nullopt;
        }
        //copy is unavoidable if the structure owns the buffer (no move)
        auto ret = std::optional<T>(m_storage[index]);
        free(index);
        return ret;
    }

    // what about just reading data without taking it?

    std::optional<T> read()
    {
        auto index = m_index.load(NO_DATA);
        if (index == NO_DATA)
        {
            return std::nullopt;
        }

        // fails: someone else may be modfiying it since we did not take the index out
        // (concurrent take, write at same index)
        return std::optional<T>(m_storage[index]);
    }

    // read buffer: assume we implement write (show), read(show)
    // ABA problem, e.g. for C = 2

    // T = (x, y);

    // buffer 73, 21@1, write?@2, write37,12@1, index changed from 1 -> 2 -> 1 during read
    // and content changed entirely from 73 to 21
    // we may have read 73 ... 12
    // or crash, if T is something like a list

    // -> T must be memcpyable and we need to detect the change!
    // if we detect a change, we just retry the operation

private:
    free(index_t index)
    {
        if (index != NO_DATA)
        {
            ptr(index)->~T();
            m_indices.free(index);
        }
    }

    T *ptr(index_t index)
    {
        return reinterpret_cast<T *>(&m_storage[index]);
    }
};
