#pragma once

#include <atomic>
#include <optional>
#include <type_traits>

#include "demo/d3_index_pool.hpp"

// lesson: manage our own memory
// lesson transfer ownership between buffers
namespace lockfree {

template <class T, uint32_t C = 8> class ReadBuffer {
private:
  using storage_t = typename std::aligned_storage<sizeof(T), alignof(T)>::type;
  using indexpool_t = IndexPool<C>;
  // LESSON: prefer indices to pointers (better control, fewer bits needed, bits
  // are precious in CAS)
  using index_t = typename indexpool_t::index_t;

  static constexpr index_t NO_DATA = C;

  std::atomic<index_t> m_index{NO_DATA};
  indexpool_t m_indices;
  storage_t m_buffer[C];

public:
  std::optional<T> write(const T &value) {
    auto maybe = m_indices.get();
    if (!maybe) {
      return std::nullopt; // no index
    }
    auto index = maybe.value();
    auto p = ptr(index);
    new (p) T(value);

    auto oldIndex = m_index.exchange(index);
    if (oldIndex != NO_DATA) {
      auto &oldValue = *ptr(oldIndex);
      auto ret = std::optional<T>(std::move(oldValue));
      free(oldIndex);
      return ret;
    }

    return std::nullopt;
  }

  bool try_write(const T &value) {
    auto maybe = m_indices.get();
    if (!maybe) {
      return false; // no index
    }
    auto index = maybe.value();
    auto p = ptr(index);
    new (p) T(value);

    index_t expected = NO_DATA;
    do {
      // LESSON: can use weak CAS in a loop
      // weak: may fail even if it contains NO_DATA ...
      if (m_index.compare_exchange_weak(expected, index)) {
        return true;
      }
    } while (expected == NO_DATA);
    free(index);
    return false;
  }

  std::optional<T> take() {
    auto index = m_index.exchange(NO_DATA);
    if (index == NO_DATA) {
      return std::nullopt;
    }
    // copy is unavoidable if the structure owns the buffer (no move)
    auto ret = std::optional<T>(std::move(*ptr(index)));
    free(index);
    return ret;
  }

  std::optional<T> read() {
    auto index = m_index.exchange(NO_DATA);
    if (index == NO_DATA) {
      return std::nullopt;
    }
    // copy is unavoidable if the structure owns the buffer (no move)
    auto ret = std::optional<T>(std::move(*ptr(index)));
    free(index);
    return ret;
  }

private:
  void free(index_t index) {
    ptr(index)->~T();
    m_indices.free(index);
  }

  T *ptr(index_t index) { return reinterpret_cast<T *>(&m_buffer[index]); }
};

} // namespace lockfree

//   // what about just reading data without taking it?

//   std::optional<T> read() {
//     auto index = m_index.load(NO_DATA);
//     if (index == NO_DATA) {
//       return std::nullopt;
//     }

//     // fails: someone else may be modfiying it since we did not take the
//     index
//     // out (concurrent take, write at same index)
//     return std::optional<T>(m_storage[index]);
//   }

//   // read buffer: assume we implement write (show), read(show)
//   // ABA problem, e.g. for C = 2

//   // T = (x, y);

//   // buffer 73, 21@1, write?@2, write37,12@1, index changed from 1 -> 2 ->
//   1
//   // during read and content changed entirely from 73 to 21 we may have
//   read 73
//   // ... 12 or crash, if T is something like a list

//   // -> T must be memcpyable and we need to detect the change!
//   // if we detect a change, we just retry the operation
