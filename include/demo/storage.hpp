#pragma once

#include <optional>
#include <type_traits>

namespace lockfree {
// assume we have this and the index pool abstraction
template<typename T, uint32_t N, typename IndexType = uint32_t> 
class Storage {
private:
  using index_t = IndexType;
  using slot_t = typename std::aligned_storage<sizeof(T), alignof(T)>::type;
  slot_t m_slots[N];

public:

  void store_at(const T &value, index_t index) {
    new(ptr(index)) T(value);
  }

  void free(index_t index) {
    ptr(index)->~T();
  }

  T *ptr(index_t index) { return reinterpret_cast<T *>(&m_slots[index]); }

  T& operator[](index_t index) {
      return *ptr(index);
  }
};

}