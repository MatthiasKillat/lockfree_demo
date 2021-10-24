#pragma once

#include <atomic>
#include <optional>

template <class T> class Interface {

  // TODO: variant where we overwrite and discard is better to present
  // exercise: variant where write returns the old value
  // replace existing value and return old value if any
  virtual bool write(const T &value) = 0;

  // write value if buffer is empty
  // return true if successful, false otherwise
  virtual bool try_write(const T &value) = 0;

  // take value from buffer (empty afterwards)
  virtual std::optional<T> take() = 0;

  // read value from buffer (value will still be in the buffer)
  virtual std::optional<T> read() = 0;
};

namespace not_lockfree {

// 1) present interface we want to implement
// write, try_write, take
#if 1
template <class T> class TakeBuffer {
private:
  // LESSON: can only modify small data atomically with lockfree operations
  std::atomic<T *> m_value{nullptr};

public:
  std::optional<T> write(const T &value) {
    // LESSON: built-in dynamic memory cannot be used
    // not lockfree in general
    auto copy = new (std::nothrow) T(value);
    if (copy == nullptr) {
      return std::nullopt; // no memory
    }

    // LESSON: prepare operation locally and finalize/make it observable with
    // a single atomic operation
    // if we do not expect to change a specific value, exchange is sufficient
    auto oldValue = m_value.exchange(copy);

    if (oldValue) {

      auto ret = std::optional<T>(std::move(*oldValue));
      delete oldValue;
      return ret;
    }

    return std::nullopt;
  }

  bool write1(const T &value) {
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

  // LESSON: built-in dynamic memory cannot be used
  // not lockfree in general

  // LESSON: prepare operation locally and finalize/make it observable with
  // a single atomic operation
  // if we do not expect to change a specific value, exchange is sufficient

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
#endif
} // namespace not_lockfree

// 5mins(10)