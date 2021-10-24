#pragma once

#include <atomic>
#include <optional>

namespace lockfree {

template <uint32_t Size> class IndexPool {
private:
  constexpr static uint8_t FREE = 0;
  constexpr static uint8_t USED = 1;

public:
  using index_t = uint32_t;

  IndexPool() {
    for (auto &slot : m_slots) {
      slot.store(FREE);
    }
  }

  std::optional<index_t> get() {
    auto startIndex = m_startIndex.load();
    auto index = startIndex;
    do {
      // LESSON: atomically obtain ownership
      auto expected = FREE;
      auto &slot = m_slots[index];
      if (slot.compare_exchange_strong(expected, USED)) {
        m_startIndex.store((index + 1) % Size);
        return index;
      }
      index = (index + 1) % Size;
      // single pass for simplicity
    } while (index != startIndex);

    return std::nullopt;
  }

  void free(index_t index) {
    auto &slot = m_slots[index];
    slot.store(FREE); // ok if there is no misuse
  }

private:
  std::atomic<uint8_t> m_slots[Size];
  std::atomic<index_t> m_startIndex{0};
}; // namespace lockfree
} // namespace lockfree

// 5mins (15)