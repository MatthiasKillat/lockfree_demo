#pragma once

#include <optional>

template <class T> class ExchangeBuffer {

  // TODO: doxygen
  // write and discard old value if any
  // returns true if successful false otherwise (leaves buffer unchanged)
  // exercise: variant where write returns the old value
  virtual bool write(const T &value) = 0;

  // write value if buffer is empty
  // return true if successful, false otherwise
  virtual bool try_write(const T &value) = 0;

  // take value from buffer (empty afterwards)
  virtual std::optional<T> take() = 0;

  // read value from buffer (value will still be in the buffer)
  virtual std::optional<T> read() = 0;
};