#pragma once

#include <atomic>
#include <optional>
#include <type_traits>

#include "lockfree/storage.hpp"
#include "lockfree/index_pool.hpp"

namespace lockfree {

template <class T, uint32_t C = 8> class TakeBuffer {
private:
  using storage_t = Storage<T, C>;
  using indexpool_t = IndexPool<C>;

  using index_t = typename indexpool_t::index_t;

  static constexpr index_t NO_DATA = C;

  std::atomic<index_t> m_index{NO_DATA};
  indexpool_t m_indices;
  storage_t m_storage;

public:  
  bool write(const T &value) {
    auto maybeIndex = m_indices.get();
    if (!maybeIndex) {
      return false; // no index
    }
    auto index = maybeIndex.value();
    // will succeed since we the exclusive ownership of index
    m_storage.store_at(value, index);

    auto oldIndex = m_index.exchange(index);
    if (oldIndex != NO_DATA) {
      free(oldIndex);
    }
    return true;
  }

  bool try_write(const T &value) {
    auto maybeIndex = m_indices.get();
    if (!maybeIndex) {
      return false; // no index
    }
    auto index = maybeIndex.value();
    m_storage.store_at(value, index);

    index_t expected = NO_DATA;
    if(!m_index.compare_exchange_strong(expected, index)) {
      free(index);
      return false;
    }
    return true;
  }

  std::optional<T> take() {
    auto index = m_index.exchange(NO_DATA);
    if (index == NO_DATA) {
      return std::nullopt;
    }    
    auto ret = std::optional<T>(std::move(m_storage[index]));
    free(index);
    return ret;
  }

  private:
    void free(index_t index) {
      m_storage.free(index);
      m_indices.free(index);
    }
};

} // namespace lockfree
