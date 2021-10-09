#pragma once

#include <atomic>

namespace lockfree
{

//simplified version
template <uint32_t Size>
class IndexPool
{
private:
    const static uint8_t FREE = 0;
    const static uint8_t USED = 1;

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

    std::optional<index_t> get_index()
    {
        auto startIndex = m_startIndex.load();
        auto index = startIndex;
        do
        {
            // LESSON: atomically obtain ownership
            auto expected = FREE;
            auto &slot = m_slots[index];
            if (slot.compare_exchange_strong(expected, USED))
            {
                m_startIndex.store((index + 1) % Size);
                return index;
            }
            index = (index + 1) % Size;
            // single pass for simplicity
        } while (index != startIndex);
    }
    return std::nullopt;
}

// O(1)
void
free_index(index_t index)
{
    auto &slot = m_slots[index];
    slot.store(FREE);
}

private:
std::atomic<uint8_t> m_slots[Size];
std::atomic<index_t> m_startIndex{0};
}; // namespace lockfree
}