#pragma once
#include <atomic>
#include <mutex>

// CAS semantics
namespace not_lockfree {

template <class T> class atomic {
  std::atomic<T> m_value;
  std::mutex m_mutex;

public:
  atomic() = default;
  atomic(const T &value) : m_value(value) {}

  T load() { return m_value.load(); }

  void store(const T &value) { m_value.store(); }

  bool compare_exchange_strong(T &expected, const T &newValue) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto value = m_value.load();
    if (m_value == expected) {
      m_value.store(newValue);
      return true;
    }

    expected = value;
    return false;
  }
};

} // namespace not_lockfree