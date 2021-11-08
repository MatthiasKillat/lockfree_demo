#pragma once

#include <atomic>
#include <chrono>
#include <cstring>
#include <optional>
#include <type_traits>

#include "lockfree/index_pool.hpp"
#include "lockfree/storage.hpp"

namespace lockfree {

template <class T, uint32_t C = 8> class ExchangeBuffer {
private:
  using storage_t = Storage<T, C>;
  using indexpool_t = lockfree::IndexPool<C>;
  using index_t = typename indexpool_t::index_t;

  static constexpr index_t NO_DATA = C;

  struct tagged_index {
    tagged_index(index_t index) : index(index) {}
    tagged_index(index_t index, uint32_t counter)
        : index(index), counter(counter) {}
    index_t index;
    uint32_t counter{0};
  };

  static_assert(std::atomic<tagged_index>::is_always_lock_free);
  static_assert(std::is_trivially_copyable<T>::value);

  std::atomic<tagged_index> m_index{NO_DATA};
  indexpool_t m_indices;
  storage_t m_storage;

public:
  bool write(const T &value) {
    auto maybeIndex = m_indices.get();
    if (!maybeIndex) {
      return false; // no index
    }
    tagged_index newIndex{maybeIndex.value()};
    m_storage.store_at(value, newIndex.index);

    tagged_index old = m_index.load();
    do {
      newIndex.counter = old.counter + 1;
      if (m_index.compare_exchange_strong(old, newIndex)) {
        if (old.index != NO_DATA) {
          free(old.index);
        }
        return true;
      }
    } while (true);
    return true;
  }

  bool try_write(const T &value) {
    auto maybeIndex = m_indices.get();
    if (!maybeIndex) {
      return false; // no index
    }
    tagged_index newIndex{maybeIndex.value()};
    m_storage.store_at(value, newIndex.index);

    tagged_index old = m_index.load();
    while (old.index == NO_DATA) {
      newIndex.counter = old.counter + 1;
      if (m_index.compare_exchange_strong(old, newIndex)) {
        return true;
      }
    }

    free(newIndex.index);
    return false;
  };

  std::optional<T> take() {
    // we basically write no data to the buffer
    // and return its content (if any)
    tagged_index newIndex(NO_DATA);
    auto old = m_index.load();

    while (old.index != NO_DATA) {
      newIndex.counter = old.counter + 1;
      if (m_index.compare_exchange_strong(old, newIndex)) {
        if (old.index != NO_DATA) {
          return std::optional<T>(std::move(m_storage[old.index]));
        } else {
          return std::nullopt;
        }
      }
    };

    return std::nullopt;
  }

  std::optional<T> read() {
    auto old = m_index.load();
    while (old.index != NO_DATA) {
      auto ret = std::optional<T>(m_storage[old.index]);

      if (m_index.compare_exchange_strong(old, old)) {
        return ret;
      }
      // if this failed either the index or the counter changed (due to a
      // concurrent write)
    }

    return std::nullopt;
  }

private:
  void free(index_t index) {
    m_storage.free(index);
    m_indices.free(index);
  }
}; // namespace lockfree

} // namespace lockfree
