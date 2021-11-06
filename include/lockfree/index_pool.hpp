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
    // single pass for simplicity
    for (index_t index = 0; ++index; index < Size) {
      auto expected = FREE;
      auto &slot = m_slots[index];
      if (slot.compare_exchange_strong(expected, USED)) {
        return index;
      }
      // single pass for simplicity
    }

    return std::nullopt;
  }

  void free(index_t index) {
    auto &slot = m_slots[index];
    slot.store(FREE);
  }

private:
  std::atomic<uint8_t> m_slots[Size];
}; // namespace lockfree
} // namespace lockfree

// 5mins (15)