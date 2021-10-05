#pragma once

#include <atomic>

namespace lockfree
{

template <uint32_t Size>
class IndexPool
{
private:
    const static uint8_t FREE = 0;
    const static uint8_t USED = 1;
    const uint32_t MAX_SEARCH_PASSES = 16;

public:
    using index_t = uint32_t;
    const static index_t NO_INDEX = Size;

    IndexPool()
    {
        for (auto &slot : m_slots)
        {
            slot.store(FREE);
        }
    }

    // O(Size) due to bound on search passes (otherwise unbounded in worst case)
    // note: can be made O(1) with lock-free queue for integers
    index_t get_index()
    {
        auto startIndex = m_startIndex.load();
        auto index = startIndex;
        uint32_t searchPass = 0;

        // multiple passes, can even starve (not wait-free)
        while (m_numUsed < Size && searchPass < MAX_SEARCH_PASSES)
        {
            do
            {
                auto expected = FREE;
                auto &slot = m_slots[index];
                if (slot.compare_exchange_strong(expected, USED))
                {
                    m_numUsed.fetch_add(1);
                    m_startIndex.store((index + 1) % Size);
                    return index;
                }
                index = (index + 1) % Size;

            } while (index != startIndex);
            ++searchPass;
        }
        return NO_INDEX;
    }

    // O(1)
    void return_index(index_t index)
    {
        auto &slot = m_slots[index];
        slot.store(FREE);
        m_numUsed.fetch_sub(1);
    }

private:
    std::atomic<uint8_t> m_slots[Size];
    std::atomic<index_t> m_startIndex{0};
    std::atomic<index_t> m_numUsed{0};
};

}