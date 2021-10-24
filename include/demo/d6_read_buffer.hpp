#pragma once

#include <atomic>
#include <chrono>
#include <cstring>
#include <optional>
#include <type_traits>

#include "demo/d3_index_pool.hpp"

namespace lockfree {

template <class T, uint32_t C = 8> class ReadBuffer {
private:
  using storage_t = typename std::aligned_storage<sizeof(T), alignof(T)>::type;
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
  storage_t m_buffer[C];

public:
  std::optional<tagged_index> copy_value(const T &value) {
    auto maybe = m_indices.get();
    if (!maybe) {
      std::nullopt;
      ;
    }
    auto index = maybe.value();
    auto p = ptr(index);
    new (p) T(value);

    return tagged_index(index);
  }

  bool write(const T &value) {

    auto maybeIndex = copy_value(value);
    if (!maybeIndex) {
      return false;
    }


    tagged_index newIndex = maybeIndex.value();
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

    return false;
  }

  

  std::optional<T> take() {

    tagged_index newIndex(NO_DATA);
    auto old = m_index.load();

    while (old.index != NO_DATA) {
      newIndex.counter = old.counter + 1;
      if (m_index.compare_exchange_strong(old, newIndex)) {
        if (old.index != NO_DATA) {
          return std::optional<T>(std::move(*ptr(old.index)));
        } else {
          return std::nullopt;
        }
      }
    };

    return std::nullopt;
  }
#if 0
  std::optional<T> take() {
    // can reset the counter
    auto old = m_index.exchange(tagged_index(NO_DATA));
    if (old.index == NO_DATA) {
      return std::nullopt;
    }
    auto ret = std::optional<T>(std::move(*ptr(old.index)));
    free(old.index);
    return ret;
  }
#endif
  std::optional<T> read1() {
    auto old = m_index.load();
    while (old.index != NO_DATA) {
      auto ret = std::optional<T>(*ptr(old.index));
      // (almost) solved the ABA problem but
      // we still invoke a copy ctor and could crash while reading

      if (m_index.compare_exchange_strong(old, old)) {
        return ret;
      }
      // if this failed either the index changed or the counter (due to a
      // concurrent write)
    }

    return std::nullopt;
  }

  std::optional<T> read2() {
    auto old = m_index.load();
    storage_t readBuffer;
    while (old.index != NO_DATA) {
      T *dest = reinterpret_cast<T *>(&readBuffer);
      // with a modified optional we could copy directly into the optional
      std::memcpy(dest, ptr(old.index), sizeof(T));
      auto ret = std::optional<T>(*dest);

      if (m_index.compare_exchange_strong(old, old)) {
        return ret;
      }
    }

    return std::nullopt;
  }

  // can we concurrently read in a lock-free manner without requiring T to be
  // trivially copyable?

private:
  void free(index_t index) {
    ptr(index)->~T();
    m_indices.free(index);
  }

  T *ptr(index_t index) { return reinterpret_cast<T *>(&m_buffer[index]); }
};

} // namespace lockfree

// 7mins (35mins)