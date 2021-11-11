#pragma once

#include <optional>

template <class T> class ExchangeBuffer {
  /// @brief write new value and discard old value if any
  /// @param value value to be written
  /// @return true if successful false otherwise (leaves buffer unchanged)
  /// @note should only be unsuccessful if the memory (usable by the buffer) is
  /// exhausted
  virtual bool write(const T &value) = 0;

  /// @brief write new value if buffer is empty
  /// @param value value to be written
  /// @return true if successful false otherwise (leaves buffer unchanged)
  virtual bool try_write(const T &value) = 0;

  /// @brief remove data from buffer and return it
  /// @return value removed or nullopt if there was no value
  virtual std::optional<T> take() = 0;

  /// @brief read data in buffer and return it
  /// @return value read or nullopt if there was no value
  virtual std::optional<T> read() = 0;
};