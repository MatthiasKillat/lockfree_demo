#pragma once

#include <atomic>
#include <optional>

namespace not_lockfree {

template <class T> class TakeBuffer {
private:
  std::atomic<T *> m_value{nullptr};

public:
  bool write(const T &value) {
    auto copy = new (std::nothrow) T(value);
    if (copy == nullptr) {
      return false;
    }

    auto oldValue = m_value.exchange(copy);

    if (oldValue) {
      delete oldValue;
    }

    return true;
  }

  std::optional<T> take() {
    auto value = m_value.exchange(nullptr);

    if (value) {
      auto ret = std::optional<T>(std::move(*value));
      delete value;
      return ret;
    }

    return std::nullopt;
  }
};
} // namespace not_lockfree